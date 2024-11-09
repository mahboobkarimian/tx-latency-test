#include <stdio.h>
#include <time.h>
#include <opencv2/opencv.hpp>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
using namespace cv;

// Function to get current time in nanoseconds
int64_t get_nanosec_timestamp() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

// Function to convert OpenCV Mat to AVFrame
AVFrame* mat_to_avframe(const Mat& mat, AVCodecContext* codec_ctx) {
    AVFrame *frame = av_frame_alloc();
    frame->format = codec_ctx->pix_fmt;
    frame->width = codec_ctx->width;
    frame->height = codec_ctx->height;
    av_image_alloc(frame->data, frame->linesize, frame->width, frame->height, codec_ctx->pix_fmt, 32);

    int cv_linesize = mat.step;
    for (int y = 0; y < mat.rows; y++) {
        memcpy(frame->data[0] + y * frame->linesize[0], mat.data + y * cv_linesize, mat.cols * 3);
    }
    return frame;
}

int main() {
    const char *filename = "output.ts";
    const int width = 640, height = 240, fps = 60;

    AVFormatContext *format_ctx = NULL;
    avformat_alloc_output_context2(&format_ctx, NULL, "mpegts", filename);
    if (!format_ctx) {
        fprintf(stderr, "Could not allocate output context\n");
        return -1;
    }

    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        return -1;
    }

    AVStream *video_stream = avformat_new_stream(format_ctx, codec);
    video_stream->time_base = (AVRational){1, fps};

    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    codec_ctx->width = width;
    codec_ctx->height = height;
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx->time_base = (AVRational){1, fps};
    codec_ctx->gop_size = fps;
    codec_ctx->max_b_frames = 0;

    if (format_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    avcodec_open2(codec_ctx, codec, NULL);
    avcodec_parameters_from_context(video_stream->codecpar, codec_ctx);

    avio_open(&format_ctx->pb, filename, AVIO_FLAG_WRITE);
    avformat_write_header(format_ctx, NULL);

    for (int i = 0; i < fps * 1000; i++) {
        int64_t timestamp_ns = get_nanosec_timestamp();
        char text[64];
        timestamp_ns /= 10000000;
        snprintf(text, sizeof(text), "%ld", timestamp_ns);
        // Create an OpenCV image with text
        Mat img(height, width, CV_8UC3, Scalar(0, 0, 0));
        putText(img, text, Point(10, height / 2), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 2);

        // Convert OpenCV Mat to AVFrame
        AVFrame *frame = mat_to_avframe(img, codec_ctx);
        frame->pts = i;

        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;

        int ret = avcodec_send_frame(codec_ctx, frame);
        if (ret < 0) {
            fprintf(stderr, "Error sending frame for encoding\n");
            break;
        }

        ret = avcodec_receive_packet(codec_ctx, &pkt);
        if (ret == 0) {
            pkt.stream_index = video_stream->index;
            av_interleaved_write_frame(format_ctx, &pkt);
            av_packet_unref(&pkt);
        } else if (ret == AVERROR(EAGAIN)) {
            continue;
        } else {
            break;
        }

        av_frame_free(&frame);  // Free the frame after it's encoded
    }

    av_write_trailer(format_ctx);
    avcodec_free_context(&codec_ctx);
    avformat_free_context(format_ctx);
    return 0;
}
