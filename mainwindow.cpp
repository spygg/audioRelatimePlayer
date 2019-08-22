#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDebug>


#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#ifdef __cplusplus
};
#endif
#endif

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

//Output PCM
#define OUTPUT_PCM 1

//Buffer:
//|-----------|-------------|
//chunk-------pos---len-----|
static  quint8   *audio_chunk;
static  quint32 audio_len;
static  quint8  *audio_pos;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initializeAudio(QAudioDeviceInfo::defaultOutputDevice());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName();



    char szFileName[255] = {0};
    memcpy(szFileName, fileName.toUtf8().constData(), fileName.length());

//    qDebug() << fileName << szFileName;
    m_generator->start();
    m_audioOutput->start(m_generator.data());



    AVFormatContext	*pFormatCtx;
    int				i, audioStream;
    AVCodecContext	*pCodecCtx;
    AVCodec			*pCodec;
    AVPacket		*packet;
    uint8_t			*out_buffer;
    AVFrame			*pFrame;

    int ret;
    uint32_t len = 0;
    int got_picture;
    int index = 0;
    int64_t in_channel_layout;
    struct SwrContext *au_convert_ctx;

    FILE *pFile=NULL;

    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    //Open
    if(avformat_open_input(&pFormatCtx,szFileName,NULL,NULL)!=0){
        qDebug() << "Couldn't open input stream"  << szFileName;
        return;
    }
    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx,NULL)<0){
        qDebug() << "Couldn't find stream information";
        return;
    }
    // Dump valid information onto standard error
    av_dump_format(pFormatCtx, 0, szFileName, false);

    // Find the first audio stream
    audioStream=-1;
    for(i=0; i < pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
            audioStream=i;
            break;
        }

    if(audioStream==-1){
        qDebug() << "Didn't find a audio stream";
        return;
    }

    // Get a pointer to the codec context for the audio stream
    pCodecCtx=pFormatCtx->streams[audioStream]->codec;

    // Find the decoder for the audio stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL){
        qDebug() << "Codec not found";
        return;
    }

    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
        qDebug() << "Could not open codec";
        return;
    }


#if OUTPUT_PCM
    pFile=fopen("xxx00oo.pcm", "wb");
#endif

    packet=(AVPacket *)av_malloc(sizeof(AVPacket));
    av_init_packet(packet);

    //Out Audio Param
    uint64_t out_channel_layout=AV_CH_LAYOUT_STEREO;
    //nb_samples: AAC-1024 MP3-1152
    int out_nb_samples=pCodecCtx->frame_size;
    AVSampleFormat out_sample_fmt=AV_SAMPLE_FMT_S16;
    int out_sample_rate=44100;
    int out_channels=av_get_channel_layout_nb_channels(out_channel_layout);
    //Out Buffer Size
    int out_buffer_size=av_samples_get_buffer_size(NULL,out_channels ,out_nb_samples,out_sample_fmt, 1);

    out_buffer=(uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);
    pFrame=av_frame_alloc();


    //FIX:Some Codec's Context Information is missing
    in_channel_layout=av_get_default_channel_layout(pCodecCtx->channels);
    //Swr

    au_convert_ctx = swr_alloc();
    au_convert_ctx=swr_alloc_set_opts(au_convert_ctx,out_channel_layout, out_sample_fmt, out_sample_rate,
        in_channel_layout,pCodecCtx->sample_fmt , pCodecCtx->sample_rate,0, NULL);
    swr_init(au_convert_ctx);

    while(av_read_frame(pFormatCtx, packet)>=0){
        if(packet->stream_index==audioStream){
            ret = avcodec_decode_audio4( pCodecCtx, pFrame,&got_picture, packet);
            if ( ret < 0 ) {
                qDebug() << "Error in decoding audio frame";
                return;
            }
            if ( got_picture > 0 ){
                swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)pFrame->data , pFrame->nb_samples);

            m_generator->setData((char*)out_buffer, out_buffer_size);
#if OUTPUT_PCM
                //Write PCM
                fwrite(out_buffer, 1, out_buffer_size, pFile);
#endif
                index++;
            }

        }
        av_free_packet(packet);
    }

    swr_free(&au_convert_ctx);

#if OUTPUT_PCM
    fclose(pFile);
#endif
    av_free(out_buffer);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);


}

void MainWindow::initializeAudio(const QAudioDeviceInfo &deviceInfo)
{
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    if (!deviceInfo.isFormatSupported(format)) {
        qWarning() << "Default format not supported - trying to use nearest";
        format = deviceInfo.nearestFormat(format);
    }

//    //设置采样率
//    audioFormat.setSampleRate(44100);
//    //设置通道数
//    audioFormat.setChannelCount(2);
//    //设置采样大小，一般为8位或16位
//    audioFormat.setSampleSize(16);
//    //设置编码方式
//    audioFormat.setCodec("audio/pcm");
//    //设置字节序
//    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
//    //设置样本数据类型
//    audioFormat.setSampleType(QAudioFormat::SignedInt);
//QAudioOutput *audio = new QAudioOutput( audioFormat, 0);
    m_generator.reset(new PcmStream());
    m_audioOutput.reset(new QAudioOutput(deviceInfo, format));

}
