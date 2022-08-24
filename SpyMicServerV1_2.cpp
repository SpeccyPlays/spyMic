/*
Receives and saves the raw audio info from an ESP32 taking samples from a MAX4466 microphone
Output file will open in Audacity as unsigned 8-bit PCM, single channel at 11,025hz (or similar)
endianness doesn't matter
Liveplays the data it receives
Displays output on screen
Press s to toggle saving to file

*/
#include <SDL.h>
#include <SDL_audio.h>
#include <SFML/Graphics.hpp>
#undef main //otherwise it expects main to be called SDL_main
#include <SFML/Network.hpp>
#include <iostream>
#include <fstream>
#define WINDOWWIDTH 256
#define WINDOWHEIGHT 256 + 64
#define BUFFERSIZE 1024
#define SAMPLEFREQUENCY 11025

void myAudioCallBack(void *userdata, uint8_t *stream, int len);
void saveToRawFile(bool saveToFile);
void drawAudioToWindow(sf::RenderWindow &window, bool saveToFile);
//below are Global as I didn't want to pass them around between functions
uint8_t receiveBuffer[BUFFERSIZE] = {0};
uint8_t drawBuffer[BUFFERSIZE] = {0};
sf::Texture redEyeTexture;
sf::Texture spyMicLogoTexture;
sf::Font recordingLabelFont;
std::ofstream outfile;

int main (){
    //Load graphics stuff
    sf::RenderWindow window(sf::VideoMode(WINDOWWIDTH, WINDOWHEIGHT), "SpyMic");
    if (!redEyeTexture.loadFromFile("images/redEye.png")){
        std::cout << "Red eye not loaded" << std::endl;
    }
    if (!spyMicLogoTexture.loadFromFile("images/spyMicLogo.png")){
        std::cout << "Logo not loaded" << std::endl;
    }
    if (!recordingLabelFont.loadFromFile("font/zxSpectrum.ttf")){
        std::cout << "Font not loaded" << std::endl;
    }
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
    //for save function
    bool saveToFile = false;
    //draw something on the window
    drawAudioToWindow(window, saveToFile);
    while(window.isOpen()){
        sf::Event event;
        while (window.pollEvent(event)){
        // event loop added otherwise window will be unresponsive
            if ((event.type == sf::Event::Closed)){
                window.close();
            }
            //check events to see if save button pressed
            sf::Keyboard keyboard;
            if (event.type == sf::Event::KeyPressed){
                if (keyboard.isKeyPressed(keyboard.S)){
                    saveToFile = !saveToFile;
                    std::cout << "S is pressed, Saving is : " << saveToFile << std::endl;
                }
            }
        }
        listener.accept(socket);
//        std::cout << "New client connected: " << socket.getRemoteAddress() << std::endl;
        // Receive data from the client
        std::size_t received = 0;
        socket.receive(receiveBuffer, sizeof(receiveBuffer), received);
        std::cout << "Received :" << received << std::endl;
        //check if we need to save or not
        saveToRawFile(saveToFile);
        //start the audio
        SDL_PauseAudio(0);
        //drawing stuff
        memcpy(drawBuffer, receiveBuffer, BUFFERSIZE);
        drawAudioToWindow(window, saveToFile);
    } //end while window openloop
    listener.close();
    SDL_Quit();
return 0;
} //end main

void drawAudioToWindow(sf::RenderWindow &window, bool saveToFile){
    /*
    Draws audio data on the screen
    We could draw the whole array but it appears that the audio sample will have duplicate info cos of the sample rate
    I make a copy of the received sample to reduce the chances trying to access it at the same time
    and make it a quarter of the size
    */
    //sample prep work to reduce the size
    uint8_t reducedDrawBuffer[BUFFERSIZE / 4] = {0};
    memcpy(reducedDrawBuffer, drawBuffer, sizeof(reducedDrawBuffer));
    window.clear(sf::Color::Black);
    //The red eye will go behind the samples
    sf::Sprite redEyeSprite;
    redEyeSprite.setTexture(redEyeTexture);
    redEyeSprite.setPosition(sf::Vector2f(96.f, 255 - 127.f)); //roughly in the middle of the samples
    window.draw(redEyeSprite);
    //Put the logo on
    sf::Sprite spyMicLogoSprite;
    spyMicLogoSprite.setTexture(spyMicLogoTexture);
    window.draw(spyMicLogoSprite);
    //Display recording message if required
    if (saveToFile){
        sf::Text text;
        text.setFont(recordingLabelFont);
        text.setString("Recording");
        text.setCharacterSize(12);
        text.setFillColor(sf::Color::Red);
        text.setPosition(sf::Vector2f(64, 64));
        window.draw(text);
    }
    //draw the samples
    sf::VertexArray line(sf::Lines, 2);
    line[0].color = sf::Color::White;
    line[1].color = sf::Color::White;
    for (int i = 1; i < (int)sizeof(reducedDrawBuffer); i++){
        line[0].position = sf::Vector2f((float)i, 255 - (float)reducedDrawBuffer[i-1]);
        line[1].position = sf::Vector2f((float)i, 255 - (float)reducedDrawBuffer[i]);
        window.draw(line);
    }

    window.display();
}
void myAudioCallBack(void *userdata, uint8_t *stream, int len){
    //called when it's reached playing the end of the audio buffer
    /*
    Compares received data against last audio stream data
    If data is the same pause the audio. It does cause a pop
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
}
void saveToRawFile(bool saveToFile){
    /*
    Saves what has been received to a file
    Checks if file open, if not open it for io. Note, if the file exists it will amend to it
    Once we no longer saving, and if it's open, close the file io
    */
    if (!outfile.is_open() && saveToFile){
        outfile.open("adc.raw", std::ios_base::out | std::ios::app | std::ios::binary);
    }
    else if (saveToFile) {
        for (uint16_t count = 0; count < sizeof(receiveBuffer); count++ ){
            uint8_t sample = (int)receiveBuffer[count]; //convert to int. Not sure if needed
                if (outfile.is_open()){
                    outfile << sample;
                }
                else {
                    std::cout << "File  didn't open" << std::endl;
                }
        } // end for loop
    } // end else loop
    else if (!saveToFile && outfile.is_open()){
        outfile.close();
        std::cout << "File is closed" << std::endl;
    }
}

