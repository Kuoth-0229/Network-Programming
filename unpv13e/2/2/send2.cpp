#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <portaudio.h>
#include <thread>

#define IMAGE_PORT 5000
#define AUDIO_PORT 5001
#define SERVER_IP "127.0.0.1"
#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512

void sendVideo() {
    #define CHUNK_SIZE 1400
    #define HEADER_SIZE 8  // 4 bytes for Frame ID, 2 for Chunk Number, 2 for Total Chunks
    
    int videoSocket = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in videoAddr = {};
    videoAddr.sin_family = AF_INET;
    videoAddr.sin_port = htons(IMAGE_PORT);
    inet_pton(AF_INET, SERVER_IP, &videoAddr.sin_addr);

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Cannot open the camera" << std::endl;
        return;
    }

    uint32_t frameID = 0;

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Failed to capture frame" << std::endl;
            break;
        }

        // Show the captured video
        //cv::imshow("Captured Video", frame);

        // Press 'q' to exit
        if (cv::waitKey(1) == 'q') {
            break;
        }

        cv::resize(frame, frame, cv::Size(640, 480));
        std::vector<int> compressionParams = {cv::IMWRITE_JPEG_QUALITY, 50};
        std::vector<uchar> buffer;
        cv::imencode(".jpg", frame, buffer, compressionParams);

        frameID++;
        size_t totalSize = buffer.size();
        size_t chunks = (totalSize + CHUNK_SIZE - HEADER_SIZE - 1) / (CHUNK_SIZE - HEADER_SIZE);

        for (size_t i = 0; i < chunks; ++i) {
            size_t start = i * (CHUNK_SIZE - HEADER_SIZE);
            size_t end = std::min(start + (CHUNK_SIZE - HEADER_SIZE), totalSize);
            size_t chunkSize = end - start;

            std::vector<uchar> packet(HEADER_SIZE + chunkSize);
            uint32_t* pFrameID = reinterpret_cast<uint32_t*>(&packet[0]);
            uint16_t* pChunkNumber = reinterpret_cast<uint16_t*>(&packet[4]);
            uint16_t* pTotalChunks = reinterpret_cast<uint16_t*>(&packet[6]);

            *pFrameID = htonl(frameID);
            *pChunkNumber = htons(i);
            *pTotalChunks = htons(chunks);

            std::copy(buffer.begin() + start, buffer.begin() + end, packet.begin() + HEADER_SIZE);

            if (sendto(videoSocket, packet.data(), packet.size(), 0, (struct sockaddr*)&videoAddr, sizeof(videoAddr)) < 0) {
                std::cerr << "Failed to send chunk " << i + 1 << "/" << chunks << ": " << strerror(errno) << std::endl;
            }
        }
    }

    close(videoSocket);
}

void sendAudio() {
    int audioSocket = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in audioAddr = {};
    audioAddr.sin_family = AF_INET;
    audioAddr.sin_port = htons(AUDIO_PORT);
    inet_pton(AF_INET, SERVER_IP, &audioAddr.sin_addr);

    // Initialize PortAudio
    Pa_Initialize();

    PaStream *stream;
    Pa_OpenDefaultStream(&stream,
                         1, // Input channels
                         0, // Output channels
                         paFloat32, // Sample format
                         SAMPLE_RATE,
                         FRAMES_PER_BUFFER,
                         nullptr, // No callback
                         nullptr); // No user data

    Pa_StartStream(stream);

    float buffer[FRAMES_PER_BUFFER];

    while (true) {
        Pa_ReadStream(stream, buffer, FRAMES_PER_BUFFER);
        if (sendto(audioSocket, buffer, sizeof(buffer), 0, (struct sockaddr *)&audioAddr, sizeof(audioAddr)) < 0) {
            std::cerr << "Failed to send audio data: " << strerror(errno) << std::endl;
            break;
        }
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    close(audioSocket);
}


int main() {


    // Threads for video and audio
    std::thread videoThread(sendVideo);
    std::thread audioThread(sendAudio);

    videoThread.join();
    audioThread.join();



    return 0;
}
