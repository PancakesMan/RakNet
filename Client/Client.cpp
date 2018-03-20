#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <iostream>
#include <thread>

#include "Client.h"
#include "Gizmos.h"
#include "Input.h"
#include "Renderer2D.h"
#include "Font.h"
#include "Texture.h"

enum GameMessages { 
	ID_SERVER_TEXT_MESSAGE = ID_USER_PACKET_ENUM + 1,
	ID_USER_TEXT_MESSAGE, ID_SET_NAME_MESSAGE,
	ID_CLIENT_CLIENT_DATA, ID_CLIENT_ID_MESSAGE,
	ID_NEW_PLAYER_MESSAGE
};

using glm::vec3;
using glm::vec4;
using glm::mat4;
using aie::Gizmos;

Client::Client() {

}

Client::~Client() {
}

bool Client::startup() {
	
	//setBackgroundColour(0.25f, 0.25f, 0.25f);

	m_font = new aie::Font("font/consolas.ttf", 32);
	m_2dRenderer = new aie::Renderer2D();

	message = "";
	chatText = new char[50];

	player = new GameObject();
	player->position = glm::vec2(getWindowWidth() / 2, getWindowHeight() / 2);
	player->colour = glm::vec4(0, 1, 0, 1);

	//// initialise gizmo primitive counts
	//Gizmos::create(10000, 10000, 10000, 10000);

	//// create simple camera transforms
	//m_viewMatrix = glm::lookAt(vec3(10), vec3(0), vec3(0, 1, 0));
	//m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f,
	//									  getWindowWidth() / (float)getWindowHeight(),
	//									  0.1f, 1000.f);

	handleNetworkConnection();

	return true;
}

void Client::shutdown() {

	Gizmos::destroy();
	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)21);
	m_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}

void Client::update(float deltaTime) {

	// query time since application started
	float time = getTime();

	// wipe the gizmos clean for this frame
	//Gizmos::clear();

	handleNetworkMessages();

	// quit if we press escape
	aie::Input* input = aie::Input::getInstance();

	// timeout to allow holding backspace
	timeout += deltaTime;

	if (timeout > cd && input->isKeyDown(aie::INPUT_KEY_BACKSPACE))
	{
		timeout = 0.0f;
		message = message.substr(0, message.length() - 1);
	}

	if (input->isKeyDown(aie::INPUT_KEY_LEFT)) {
		player->position.x -= 100.0f * deltaTime;
		sendClientGameObject();
	}

	if (input->isKeyDown(aie::INPUT_KEY_RIGHT)) {
		player->position.x += 100.0f * deltaTime;
		sendClientGameObject();
	}

	if (input->isKeyDown(aie::INPUT_KEY_UP)) {
		player->position.y += 100.0f * deltaTime;
		sendClientGameObject();
	}

	if (input->isKeyDown(aie::INPUT_KEY_DOWN)) {
		player->position.y -= 100.0f * deltaTime;
		sendClientGameObject();
	}

	// Place pressed characters (as lowercase) into command
	for (auto c : input->getPressedCharacters())
		message += (char)c;

	if (input->wasKeyPressed(aie::INPUT_KEY_ENTER))
	{
		if (input->isKeyDown(aie::INPUT_KEY_LEFT_SHIFT) || input->isKeyDown(aie::INPUT_KEY_RIGHT_SHIFT))
		{
			RakNet::BitStream bs;
			bs.Write((RakNet::MessageID)GameMessages::ID_SET_NAME_MESSAGE);
			bs.Write(message.c_str());

			m_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
		}
		else
		{
			RakNet::BitStream bs;
			bs.Write((RakNet::MessageID)GameMessages::ID_USER_TEXT_MESSAGE);
			bs.Write(message.c_str());

			m_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
		}

		message = "";
	}

	if (input->isKeyDown(aie::INPUT_KEY_ESCAPE))
		quit();
}

void Client::draw() {

	// wipe the screen to the background colour
	clearScreen();
	m_2dRenderer->begin();
	//// update perspective in case window resized
	//m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f,
	//									  getWindowWidth() / (float)getWindowHeight(),
	//									  0.1f, 1000.f);

	//Gizmos::draw(m_projectionMatrix * m_viewMatrix);

	m_2dRenderer->setRenderColour(1, 1, 1, 1);
	m_2dRenderer->drawText(m_font, message.c_str(), 0, 10);
	m_2dRenderer->drawText(m_font, std::string("_").c_str(), m_font->getStringWidth(message.c_str()), 10);

	m_2dRenderer->setRenderColour(
		player->colour.x,
		player->colour.y,
		player->colour.z,
		player->colour.w
	);
	m_2dRenderer->drawCircle(player->position.x, player->position.y, 20);

	for (auto it = players.begin(); it != players.end(); it++)
	{
		m_2dRenderer->setRenderColour(
			it->second.colour.x,
			it->second.colour.y,
			it->second.colour.z,
			it->second.colour.w
		);
		m_2dRenderer->drawCircle(it->second.position.x, it->second.position.y, 20);
	}

	m_2dRenderer->end();
}

void Client::handleNetworkConnection()
{
	m_pPeerInterface = RakNet::RakPeerInterface::GetInstance();
	initializeClientConnection();
}

void Client::initializeClientConnection()
{
	//Create a socket descriptor to describe this connection
	//No data needed, as we will be connecting to a server
	RakNet::SocketDescriptor sd;

	// Now call startup - max of 1 connection to the server
	m_pPeerInterface->Startup(1, &sd, 1);

	std::cout << "Connecting to the server at " << IP << "...\n";

	// Now call connect to attemp to connect to the given server
	RakNet::ConnectionAttemptResult res = m_pPeerInterface->Connect(IP, PORT, nullptr, 0);

	// Finally, chec to see if we connected, and if not, throw an error
	if (res != RakNet::CONNECTION_ATTEMPT_STARTED)
		std::cout << "Unable to start connection, Error number: " << res << std::endl;
	else
		sendClientGameObject();
}

void Client::handleNetworkMessages()
{
	RakNet::Packet* packet;

	for (packet = m_pPeerInterface->Receive(); packet;
		m_pPeerInterface->DeallocatePacket(packet),
		packet = m_pPeerInterface->Receive())
	{
		switch (packet->data[0])
		{
		case ID_REMOTE_DISCONNECTION_NOTIFICATION: 
		{
			std::cout << "Another client has disconnected.\n";
			RakNet::BitStream bsIn(packet->data, packet->length, false);
			bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
			RakNet::SystemAddress addr;
			bsIn.Read(addr);

			auto it = players.find(addr);
			players.erase(it);

			break;
		}
		case ID_REMOTE_CONNECTION_LOST: 
			std::cout << "Another client has lost the connection.\n"; 
			break; 
		case ID_REMOTE_NEW_INCOMING_CONNECTION: 
			std::cout << "Another client has connected.\n"; 
			sendClientGameObject();
			break; 
		case ID_CONNECTION_REQUEST_ACCEPTED: 
			std::cout << "Our connection request has been accepted.\n"; 
			break; 
		case ID_NO_FREE_INCOMING_CONNECTIONS: 
			std::cout << "The server is full.\n"; 
			break; 
		case ID_DISCONNECTION_NOTIFICATION: 
			std::cout << "We have been disconnected.\n"; 
			break; 
		case ID_CONNECTION_LOST: 
			std::cout << "Connection lost.\n"; 
			break; 
		case ID_SERVER_TEXT_MESSAGE:
		{
			RakNet::BitStream bsIn(packet->data, packet->length, false);
			bsIn.IgnoreBytes(sizeof(RakNet::MessageID));

			//RakNet::SystemAddress name;
			RakNet::RakString name, message;
			bsIn.Read(name);
			bsIn.Read(message);

			std::cout << name.C_String() << " said: " << message.C_String() << std::endl;
			break;
		}
		case ID_CLIENT_CLIENT_DATA:
		{
			RakNet::BitStream bsIn(packet->data, packet->length, false);
			bsIn.IgnoreBytes(sizeof(RakNet::MessageID));

			RakNet::SystemAddress player;
			bsIn.Read(player);

			if (player != me)
			{
				GameObject data;
				bsIn.Read((char*)&data, sizeof(GameObject));

				players[player] = data;
				//std::cout << "Client " << player.ToString() << " at position: " << data.position.x << "," << data.position.y << std::endl;
			}
			break;
		}
		case ID_CLIENT_ID_MESSAGE:
		{
			RakNet::BitStream bsIn(packet->data, packet->length, false);
			bsIn.IgnoreBytes(sizeof(RakNet::MessageID));

			bsIn.Read(me);
			break;
		}
		default: 
			std::cout << "Received a message with a unknown id: " << packet->data[0]; 
			break;
		}
	}
}

void Client::sendClientGameObject()
{
	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)GameMessages::ID_CLIENT_CLIENT_DATA);
	bs.Write((char*)player, sizeof(GameObject));

	m_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}
