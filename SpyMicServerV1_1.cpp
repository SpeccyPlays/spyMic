/*
Receives and saves the raw audio info from an ESP32 taking samples from a MAX4466 microphone
Output file will open in Audacity as unsigned 8-bit PCM, single channel at 11,025hz (or similar)
endianness doesn't matter
Liveplays the data it receives

*/
#include <SDL.h>
#include <SDL_audio.h>
#include <SFML/Graphics.hpp>
#undef main //otherwise it expects main to be called SDL_main
#include <SFML/Network.hpp>
#include <iostream>
#include <fstream>

#define BUFFERSIZE 1024
#define SAMPLEFREQUENCY 11025

void myAudioCallBack(void *userdata, uint8_t *stream, int len);
void saveToRawFile();
void drawAudioToWindow(sf::RenderWindow &window);
uint8_t receiveBuffer[BUFFERSIZE] = {0};
uint8_t drawBuffer[BUFFERSIZE] = {0};

int main (){
    //setup display window
    sf::RenderWindow window(sf::VideoMode(1024, 300), "SpyMic");
    //setup all audio settings
    SDL_Init(SDL_INIT_AUDIO);
    SDL_AudioSpec audiospec;
    audiospec.freq = SAMPLEFREQUENCY;
    audiospec.format = AUDIO_U8;
    audiospec.channels = 1;
    audiospec.silence = 127;
    audiospec.samples = BUFFERSIZE;
    audiospec.callback = myAudioCallBack;
    audiospec.userdata = NULL;
    SDL_OpenAudio(&audiospec, NULL); //pass as reference needed
    //setup TCP requirements
    sf::TcpListener listener;
    listener.listen(8090);
    sf::TcpSocket socket;
    socket.setBlocking(true);
    bool saveToFile = false;
    while(window.isOpen()){
        sf::Event event;
//        window.clear(sf::Color::Black);
//        window.display();
        while (window.pollEvent(event)){
            if ((event.type == sf::Event::Closed)){
                window.close();
            }
        // event loop added otherwise window will be unresponsive
        }
        listener.accept(socket);
        std::cout << "New client connected: " << socket.getRemoteAddress() << std::endl;
        // Receive data from the client
        std::size_t received = 0;
        socket.receive(receiveBuffer, sizeof(receiveBuffer), received);
        std::cout << "Received :" << received << std::endl;
        if (saveToFile){
           saveToRawFile();
        }
        SDL_PauseAudio(0);
        //drawing stuff
        memcpy(drawBuffer, receiveBuffer, BUFFERSIZE);
        drawAudioToWindow(window);
    } //end while window openloop
    listener.close();
    SDL_Quit();
return 0;
} //end main

void drawAudioToWindow(sf::RenderWindow &window){
    /*
    Draws audio data on the screen
    */
    window.clear(sf::Color::Black);
    sf::VertexArray line(sf::Lines, 2);
    line[0].color = sf::Color::White;
    line[1].color = sf::Color::White;
    for (int i = 0; i < (int)sizeof(drawBuffer); i++){
            line[0].position = sf::Vector2f((float)i, 255);
            line[1].position = sf::Vector2f((float)i, 255 - (float)drawBuffer[i]);
            window.draw(line);
    }
    window.display();
}

void myAudioCallBack(void *userdata, uint8_t *stream, int len){
    //called when it's reached playing the end of the audio buffer
    /*
    Compares received data against last audio stream data
    then cuts audio if they're the same - does cause a pop sometimes when it's cut.
    If only memcpy here then it will loop the last received audio
    I found that annoying if the ESP32's network kept dropping
    */
    int memcmpResult = memcmp(stream, receiveBuffer, sizeof(receiveBuffer));
    if (memcmpResult == 0){
        std::cout << "Avoiding replaying audio" << std::endl;
        SDL_PauseAudio(1);
    }
    else {
        memcpy(stream, &receiveBuffer, sizeof(receiveBuffer)); //leave this even if everything else deleted
    }
// memcpy(stream, &receiveBuffer, sizeof(receiveBuffer));
}
void saveToRawFile(){
    /*
    * Saves what has been received to a file
    */
    std::ofstream outfile;
    outfile.open("adc.raw", std::ios_base::out | std::ios::app | std::ios::binary);
    for (uint16_t count = 0; count < sizeof(receiveBuffer); count++ ){
        uint8_t sample = (int)receiveBuffer[count]; //convert to int. Not sure if needed
            if (outfile.is_open()){
                outfile << sample;
            }
            else {
                std::cout << "File  didn't open" << std::endl;
            }
    } // end for loop
    outfile.close();
}
