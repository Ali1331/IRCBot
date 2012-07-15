#include <stdio.h>
#include <winsock2.h>
#include <string>
#include <vector>

using namespace std;

SOCKET sock;
string prefix;
string nick = "Ali1332";
string hostname = "irc.freenode.net";
int port = 6667;
vector<string> channels(1, "##ali");

string getErrorMessage(int errorID) {
	char r[128];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
				  0,
				  errorID,
				  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				  (LPTSTR)r,
				  128,
				  NULL);
	return string(r);
}

string getUserFromHost(string hostname) {
    return hostname.substr(hostname[0] == ':' ? 1 : 0, hostname.find('!')-1);
}

string strjoin(vector<string> tokens, char delim, int start=0) {
    string m = tokens[start++];
    while(start < tokens.size()) {
        m += " "+tokens[start];
        start++;
    }
    return m;
}

vector<string> strsplit(string c, char delim) {
    vector<string> tokens;
    int start = 0;
    int end = 0;

    int i = 0;
    while(i < c.size()) {
        if(c.at(i) == delim) {
            if(start != i) {
                tokens.push_back(c.substr(start, i-start));
                start = i+1;
            }
        }
        i++;
    }
    tokens.push_back(c.substr(start, i-start));
    return tokens;
}

int sendMessage(string message) {
    message += "\r\n";
    return send(sock, message.c_str(), message.size(), 0);
}

string getMessage() {
    string buffer;
    char cur;

    while(recv(sock, &cur, 1, 0)) {
        if(cur == '\n') {
            if(buffer.at(buffer.size()-1) == '\r') {
                buffer += '\r';
                break;
            }
        }
        buffer += cur;
    }
    return buffer;
}

int parseMessage(vector<string> tokens) {
    string command = tokens[1];
    if(tokens[0] == "PING") {
        sendMessage("PONG "+tokens[1]);
        return 1;
    }
    string user = getUserFromHost(tokens[0]);
    string channel = tokens[2];
    if(command == "MODE") {
        string modes = strjoin(tokens, ' ', 3);
        printf("%s set mode: %s\n", user.c_str(), (modes[0] == ':' ? modes.substr(1, -1) : modes).c_str());
        return 1;
    }
    if(command == "JOIN") {
        printf("%s joined %s\n", user.c_str(), channel.c_str());
        return 1;
    }
    if(command == "PART") {
        printf("%s left %s\n", user.c_str(), channel.c_str());
        return 1;
    }
    if(command == "PRIVMSG") {
        printf("%s %s: %s\n", channel.c_str(),user.c_str(), strjoin(tokens, ' ', 3).substr(1, -1).c_str());
        if(tokens[3].substr(1,4) == "!hop" && user == "Ali1331") {
            sendMessage("PART "+channel);
            Sleep(250);
            sendMessage("JOIN "+channel);
        }
        return 1;
    }
    if(tokens[0] == prefix) {
        return 1;
    }
    return 0;
}

int main()
{
	WSAData wsaData;
	int nCode;

	if((nCode = WSAStartup(MAKEWORD(2,2), &wsaData)) != 0) {
		printf("WSAStartup() returned error code %d.\n", getErrorMessage(nCode).c_str());
		WSACleanup();
		return -1;
	}

	sock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(sock == INVALID_SOCKET) {
		printf("Error opening socket: %d\n", getErrorMessage(WSAGetLastError()).c_str());
		WSACleanup();
		return -1;
	}

	struct hostent *host;
	if(!(host = gethostbyname(hostname.c_str()))) {
		printf("Failed to resolve hostname.\r\n");
		WSACleanup();
		return -1;
	}

	SOCKADDR_IN sockAddr;
	sockAddr.sin_port = htons(port);
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);

	if(connect(sock, (SOCKADDR*)(&sockAddr), sizeof(sockAddr))) {
		printf("Failed to establish connection with server: %s\r\n", getErrorMessage(WSAGetLastError()).c_str());
		WSACleanup();
		return -1;
	}

	sendMessage("USER "+nick+" d d ali");
	sendMessage("NICK "+nick);

    vector<string> firstChunks = strsplit(getMessage(), ' ');
    prefix = firstChunks.front();

    int len = prefix.size();
    while(1) {
        string code = getMessage().substr(len+1, 3);
        if(code == "422" || code == "376") {
            printf("Connected to %s on port %d\n", hostname.c_str(), port);
            break;
        }
    }

    for(int i=0; i<channels.size(); i++) {
        sendMessage("JOIN "+channels[i]);
    }

	while(1) {
		string buffer = getMessage();

		if(!parseMessage(strsplit(buffer, ' '))) {
            printf("UNHANDLED: %s\n", buffer.c_str());
		}

		int error = WSAGetLastError();
		if(error != 0) {
			printf("Error: %s\n", getErrorMessage(error).c_str());
			break;
		}
	}

	shutdown(sock, SD_SEND);
	closesocket(sock);
	WSACleanup();

	return 0;
}
