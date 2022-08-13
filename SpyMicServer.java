/*
 * Receives sound data sent from MAX4466 connected to ESP32 and plays it
 */


package spyMicServer;

import java.io.*;
import java.net.*;
import javax.sound.sampled.*;

public class SpyMicServer {
	static int portNumber = 8090;
	int bufferSize = 1024;
	public void run(int port) {
		try (ServerSocket serverSocket = new ServerSocket(port)) {
			System.out.println("Server is listening on port " + port);
			byte[] receiveBuffer = new byte[bufferSize];
			Socket clientSocket = serverSocket.accept();
			DataInputStream recData = new DataInputStream(clientSocket.getInputStream());
			recData.readFully(receiveBuffer);
			AudioFormat format = new AudioFormat(11025.f,  8, 1, false, true);
			try {
				Clip soundToPlay = AudioSystem.getClip();
				soundToPlay.open(format, receiveBuffer, 0, bufferSize);
				soundToPlay.start();
			} catch (LineUnavailableException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			serverSocket.close();
		}
		catch (IOException ex) {
            System.out.println("Server exception: " + ex.getMessage());
            ex.printStackTrace();
        }
	}

	public static void main(String[] args) {
		// TODO Auto-generated method stub
		SpyMicServer newServer = new SpyMicServer();
		while(true) {
			newServer.run(portNumber);
		}
	}

}
