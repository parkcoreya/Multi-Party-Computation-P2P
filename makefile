
CC  = g++
OBJ = MPC_PeerCommon.o MPC_PeerHandler.o MPC_PeerShare.o MPC_ReadConfig.o \
      MPC_PeerConnection.o MPC_Peer.o MPC_PolyModule.o MPC_PeerTest.o
BIN = netPeer

CFLAGS = -std=c++11 -g -Wno-pmf-conversions
LFLAGS = -lstdc++ -lpthread 

all:	$(BIN)
	g++ $(OBJ) -o $(BIN) $(LFLAGS)

clean:
	rm -f $(OBJ)

distclean:
	rm -f $(OBJ) $(BIN)

$(BIN): $(OBJ)

MPC_PeerCommon.o: MPC_PeerCommon.cc
	$(CC) -c MPC_PeerCommon.cc $(CFLAGS)

MPC_PeerHandler.o: MPC_PeerHandler.cc
	$(CC) -c MPC_PeerHandler.cc $(CFLAGS)

MPC_PeerShare.o: MPC_PeerShare.cc
	$(CC) -c MPC_PeerShare.cc $(CFLAGS)

MPC_ReadConfig.o: MPC_ReadConfig.cc
	$(CC) -c MPC_ReadConfig.cc $(CFLAGS)

MPC_PeerConnection.o: MPC_PeerConnection.cc
	$(CC) -c MPC_PeerConnection.cc $(CFLAGS)

MPC_Peer.o: MPC_Peer.cc
	$(CC) -c MPC_Peer.cc $(CFLAGS)

MPC_PolyModule.o: MPC_PolyModule.cc
	$(CC) -c MPC_PolyModule.cc $(CFLAGS)

MPC_PeerTest.o: MPC_PeerTest.cc
	$(CC) -c MPC_PeerTest.cc $(CFLAGS)


SRCS = `echo ${OBJ} | sed -e 's/.o /.cc /g'`
depend:
	@echo ${SRCS}
	makedepend -Y $(SRCS)
# DO NOT DELETE

MPC_PeerCommon.o: MPC_PeerCommon.h MPC_Common.h
MPC_PeerHandler.o: MPC_PeerHandler.h MPC_Peer.h MPC_PeerCommon.h MPC_Common.h
MPC_PeerHandler.o: MPC_PeerConnection.h MPC_PeerShare.h MPC_PolyModule.h
MPC_PeerShare.o: MPC_PeerShare.h MPC_PeerCommon.h MPC_Common.h
MPC_PeerShare.o: MPC_PolyModule.h
MPC_ReadConfig.o: MPC_Common.h MPC_ReadConfig.h
MPC_PeerConnection.o: MPC_PeerConnection.h MPC_PeerCommon.h MPC_Common.h
MPC_Peer.o: MPC_Peer.h MPC_PeerCommon.h MPC_Common.h MPC_PeerConnection.h
MPC_Peer.o: MPC_PeerShare.h MPC_PolyModule.h
MPC_PolyModule.o: MPC_PolyModule.h MPC_Common.h
