/*
Receives and saves the raw audio info from an ESP32 taking samples from a MAX4466 microphone
Output file will open in Audacity as unsigned 8-bit PCM, single channel at 11,025hz (or similar)
endianness doesn't matter
Liveplays the data it receives

*/
#include <SDL.h>
#include <SDL_audio.h>
#undef main //otherwise it expects main to be called SDL_main
#include <SFML/Network.hpp>
#include <iostream>
#include <fstream>

#define BUFFERSIZE 1024

void myAudioCallBack(void *userdata, uint8_t *stream, int len);
uint8_t receiveBuffer[BUFFERSIZE] = {0};

int main (){
    std::ofstream outfile;
    std::ofstream outfile16;
    // setup all audio settings
    SDL_Init(SDL_INIT_AUDIO);
    SDL_AudioSpec audiospec;
    audiospec.freq = 11025;
    audiospec.format = AUDIO_U8;
    audiospec.channels = 1;
    audiospec.silence = 127;
    audiospec.samples = BUFFERSIZE;
    audiospec.callback = myAudioCallBack;
    audiospec.userdata = NULL;
    SDL_OpenAudio(&audiospec, NULL); //pass as reference needed

    while(1){
        //open file
        outfile.open("adc.raw", std::ios_base::out | std::ios::app | std::ios::binary);
        // Create a listener to wait for incoming connections on port
        sf::TcpListener listener;
        listener.listen(8090);
        // Wait for a connection
        sf::TcpSocket socket;
        socket.setBlocking(true); //if not blocking it wasn't receiving all data
        listener.accept(socket);
        std::cout << "New client connected: " << socket.getRemoteAddress() << std::endl;
        // Receive data from the client
        std::size_t received = 0;
        socket.receive(receiveBuffer, sizeof(receiveBuffer), received);
        std::cout << "Received :" << received << std::endl;
        for (uint16_t count = 0; count < sizeof(receiveBuffer); count++ ){
            uint8_t sample = (int)receiveBuffer[count]; //convert to int. Not sure if needed
            if (outfile.is_open()){
                outfile << sample;
            }
            else {
                std::cout << "File  didn't open" << std::endl;
            }
        }
        outfile.close();
        SDL_PauseAudio(0);
    } //end while loop
    SDL_Quit();
return 0;
} //end main

void myAudioCallBack(void *userdata, uint8_t *stream, int len){
    //called when it's reached playing the end of the audio buffer
    memcpy(stream, &receiveBuffer, sizeof(receiveBuffer));
}


