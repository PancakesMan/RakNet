#include <iostream>
#include <string>
#include <thread>
#include <map>

#include <RakPeerInterface.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>

#include "GameMessages.h"
#include "GameObject.h"

void handleNetworkMessages(RakNet::RakPeerInterface*);
void sendClientPing(RakNet::RakPeerInterface*);

int main()
{
	const unsigned short PORT = 5456;
	RakNet::RakPeerInterface* pPeerInterface = nullptr;

	std::cout << "Starting up the server..." << std::endl;

	// Initialize the Raknet peer interface
	pPeerInterface = RakNet::RakPeerInterface::GetInstance();

	// Create a socket descriptor to describe this connection
	RakNet::SocketDescriptor sd(PORT, 0);

	// Now call startup - max of 32 connections on the assigned port
	pPeerInterface->Startup(32, &sd, 1);
	pPeerInterface->SetMaximumIncomingConnections(32);

	//std::thread pingThread(sendClientPing, pPeerInterface);
	handleNetworkMessages(pPeerInterface);

	return 0;
}

void handleNetworkMessages(RakNet::RakPeerInterface* pPeerInterface)
{
	std::map<RakNet::SystemAddress, std::string> usernames;

	RakNet::Packet* packet = nullptr;
	for (;;) {
		for (packet = pPeerInterface->Receive(); packet;
			pPeerInterface->DeallocatePacket(packet),
			packet = pPeerInterface->Receive())
		{
			switch (packet->data[0])
			{
			case ID_NEW_INCOMING_CONNECTION:
			{
				std::cout << "A connection is incoming from: " << packet->systemAddress.ToString() << "\n";
				usernames[packet->systemAddress] = "user" + std::to_string(packet->systemAddress.GetPort());

				RakNet::BitStream bs;
				bs.Write((RakNet::MessageID)GameMessages::ID_CLIENT_ID_MESSAGE);
				bs.Write(packet->systemAddress);
				pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);

				RakNet::BitStream bs2;
				bs2.Write((RakNet::MessageID)ID_REMOTE_NEW_INCOMING_CONNECTION);
				pPeerInterface->Send(&bs2, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);

				break;
			}
			case ID_DISCONNECTION_NOTIFICATION:
			{
				std::cout << "The client at " << packet->systemAddress.ToString() << " has disconnected.\n";
				auto it = usernames.find(packet->systemAddress);
				usernames.erase(it);

				RakNet::BitStream bs;
				bs.Write((RakNet::MessageID)31);
				bs.Write(packet->systemAddress);
				pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);

				break;
			}
			case ID_CONNECTION_LOST:
			{
				std::cout << "The client at " << packet->systemAddress.ToString() << " has lost the connection.\n";
				auto it = usernames.find(packet->systemAddress);
				usernames.erase(it);

				RakNet::BitStream bs;
				bs.Write((RakNet::MessageID)31);
				bs.Write(packet->systemAddress);
				pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);

				break;
			}
			case ID_USER_TEXT_MESSAGE:
			{
				RakNet::BitStream bsIn(packet->data, packet->length, false);
				bsIn.IgnoreBytes(sizeof(RakNet::MessageID));

				RakNet::RakString str;
				bsIn.Read(str);

				RakNet::BitStream bs;
				bs.Write((RakNet::MessageID)GameMessages::ID_SERVER_TEXT_MESSAGE);
				bs.Write(RakNet::RakString(usernames[packet->systemAddress].c_str()));
				bs.Write(str);

				pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
				break;
			}
			case ID_SET_NAME_MESSAGE:
			{
				RakNet::BitStream bsIn(packet->data, packet->length, false);
				bsIn.IgnoreBytes(sizeof(RakNet::MessageID));

				RakNet::RakString str;
				bsIn.Read(str);

				usernames[packet->systemAddress] = str.C_String();
				break;
			}
			case ID_CLIENT_CLIENT_DATA:
			{
				GameObject object;
				RakNet::BitStream bsIn(packet->data, packet->length, false);
				bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
				bsIn.Read((char*)&object, sizeof(GameObject));

				RakNet::BitStream bs;
				bs.Write((RakNet::MessageID)GameMessages::ID_CLIENT_CLIENT_DATA);
				bs.Write(packet->systemAddress);
				bs.Write((char*)&object, sizeof(GameObject));

				pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
				break;
			}
			default:
				std::cout << "Received a message with an unknwon id: " << packet->data[0] << "\n";
				break;
			}
		}
	}
}

void sendClientPing(RakNet::RakPeerInterface* pPeerInterface)
{
	for (;;) {
		RakNet::BitStream bs;
		bs.Write((RakNet::MessageID)GameMessages::ID_SERVER_TEXT_MESSAGE);
		bs.Write("Ping!");

		pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}