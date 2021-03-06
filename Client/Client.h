#pragma once

#include <string>
#include <map>

#include "Application.h"
#include "Renderer2D.h"
#include "GameObject.h"

#include <glm/mat4x4.hpp>

#include <RakPeerInterface.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>

class Client : public aie::Application {
public:

	Client();
	virtual ~Client();

	virtual bool startup();
	virtual void shutdown();

	virtual void update(float deltaTime);
	virtual void draw();

	// Initialize the connection
	void handleNetworkConnection();
	void initializeClientConnection();

	// Handle incoming packets
	void handleNetworkMessages();

	// Send GameObject data to server
	void sendClientGameObject();

protected:

	glm::mat4	m_viewMatrix;
	glm::mat4	m_projectionMatrix;

	RakNet::RakPeerInterface* m_pPeerInterface;

	aie::Font* m_font;
	aie::Renderer2D* m_2dRenderer;

	const char* IP = "127.0.0.1";
	const unsigned short PORT = 5456;

	std::string message;
	std::map<RakNet::SystemAddress, GameObject> players;

	const float cd = 0.12f;
	float timeout = 0.0f;

	char* chatText;

	GameObject* player;

	RakNet::SystemAddress me;
};