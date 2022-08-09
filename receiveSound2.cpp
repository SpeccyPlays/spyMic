/*
Receives and saves the raw audio info from an ESP32 taking samples from a MAX4466 microphone
Output file will open in Audacity as unsigned 8-bit PCM, single channel at 11,025hz (or similar)
endianness doesn't matter
*/

#include <SFML/Network.hpp>
#include <iostream>
#include <fstream>
#define BUFFERSIZE 1024


int main (){
    uint8_t soundData[BUFFERSIZE];
    std::ofstream outfile;
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
        // Receive a message from the client
        uint8_t receiveBuffer[BUFFERSIZE];
        std::size_t received = 0;
        socket.receive(receiveBuffer, sizeof(receiveBuffer), received);
        std::cout << "Received :" << received << std::endl;
        for (uint16_t count = 0; count < sizeof(receiveBuffer); count++){
            soundData[count] = (int)receiveBuffer[count];// cast makes sure it's not copying binary data.
            if (outfile.is_open()){
                outfile << soundData[count];
            }
            else {
                std::cout << "File didn't open" << std::endl;
            }
        }
        outfile.close();
    } //end while loop
return 0;
} //end main


