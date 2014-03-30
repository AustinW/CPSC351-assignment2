all: receiver sender

# Receiver
receiver: recv.cpp
	g++ recv.cpp -o receiver

# Sender
sender: sender.cpp
	g++ sender.cpp -o sender