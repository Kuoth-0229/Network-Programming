#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <portaudio.h>

#define IMAGE_PORT 5000
#define AUDIO_PORT 5001
#define BUFFER_SIZE 1400
#define HEADER_SIZE 8  // 4 bytes for Frame ID, 2 for Chunk Number, 2 for Total Chunks
#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512

std::queue<cv::Mat> frameQueue;
std::mutex queueMutex;
std::condition_variable frameCondVar;
std::atomic<bool> stopFlag(false);

void receiveVideo() {
    int videoSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (videoSocket < 0) {
        std::cerr << "Failed to create socket." << std::endl;
        return;
    }

    struct sockaddr_in videoAddr = {};
    videoAddr.sin_family = AF_INET;
    videoAddr.sin_port = htons(IMAGE_PORT);
    videoAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(videoSocket, (struct sockaddr *)&videoAddr, sizeof(videoAddr)) < 0) {
        std::cerr << "Failed to bind socket." << std::endl;
        close(videoSocket);
        return;
    }

    std::unordered_map<uint32_t, std::vector<std::vector<uchar>>> frameChunks;
    std::unordered_map<uint32_t, size_t> frameChunkCounts;

    while (!stopFlag) {
        std::vector<uchar> packet(BUFFER_SIZE);
        ssize_t receivedSize = recvfrom(videoSocket, packet.data(), BUFFER_SIZE, 0, nullptr, nullptr);
        if (receivedSize < 0) {
            std::cerr << "Failed to receive data: " << strerror(errno) << std::endl;
            continue;
        }

        uint32_t frameID = ntohl(*reinterpret_cast<uint32_t*>(&packet[0]));
        uint16_t chunkNumber = ntohs(*reinterpret_cast<uint16_t*>(&packet[4]));
        uint16_t totalChunks = ntohs(*reinterpret_cast<uint16_t*>(&packet[6]));

        std::vector<uchar> chunk(packet.begin() + HEADER_SIZE, packet.begin() + receivedSize);

        frameChunks[frameID].resize(totalChunks);
        frameChunks[frameID][chunkNumber] = std::move(chunk);
        frameChunkCounts[frameID]++;

        if (frameChunkCounts[frameID] == totalChunks) {
            std::vector<uchar> completeBuffer;
            for (const auto& chunk : frameChunks[frameID]) {
                completeBuffer.insert(completeBuffer.end(), chunk.begin(), chunk.end());
            }

            cv::Mat frame = cv::imdecode(completeBuffer, cv::IMREAD_COLOR);
            if (!frame.empty()) {
                std::unique_lock<std::mutex> lock(queueMutex);
                frameQueue.push(frame);
                frameCondVar.notify_one();
            }

            frameChunks.erase(frameID);
            frameChunkCounts.erase(frameID);
        }
    }

    close(videoSocket);
}

void displayFrames() {
    while (!stopFlag) {
        std::unique_lock<std::mutex> lock(queueMutex);
        frameCondVar.wait(lock, [] { return !frameQueue.empty() || stopFlag; });

        while (!frameQueue.empty()) {
            cv::Mat frame = frameQueue.front();
            frameQueue.pop();
            lock.unlock();

            cv::imshow("Received Frame", frame);
            if (cv::waitKey(1) == 27) {  // Stop on ESC key
                stopFlag = true;
                return;
            }

            lock.lock();
        }
    }
}




void receiveAudio() {
    int audioSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (audioSocket < 0) {
        std::cerr << "Failed to create audio socket." << std::endl;
        return;
    }

    struct sockaddr_in audioAddr = {};
    audioAddr.sin_family = AF_INET;
    audioAddr.sin_port = htons(AUDIO_PORT);
    audioAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(audioSocket, (struct sockaddr *)&audioAddr, sizeof(audioAddr)) < 0) {
        std::cerr << "Failed to bind audio socket." << std::endl;
        close(audioSocket);
        return;
    }

    // Initialize PortAudio
    Pa_Initialize();

    PaStream *stream;
    Pa_OpenDefaultStream(&stream,
                         0, // Input channels
                         1, // Output channels
                         paFloat32, // Sample format
                         SAMPLE_RATE,
                         FRAMES_PER_BUFFER,
                         nullptr, // No callback
                         nullptr); // No user data

    Pa_StartStream(stream);

    float buffer[FRAMES_PER_BUFFER];
    while (!stopFlag) {
        ssize_t receivedSize = recvfrom(audioSocket, buffer, sizeof(buffer), 0, nullptr, nullptr);
        if (receivedSize < 0) {
            std::cerr << "Failed to receive audio data: " << strerror(errno) << std::endl;
            break;
        }
        Pa_WriteStream(stream, buffer, FRAMES_PER_BUFFER);
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    close(audioSocket);
}




int main() {
    std::thread videoThread(receiveVideo);
    std::thread audioThread(receiveAudio);

    // Display frames on the main thread
    displayFrames();

    stopFlag = true;
    videoThread.join();
    audioThread.join();

    return 0;
}
