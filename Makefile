CC = g++
CFLAGSSQL = -I/usr/include/mysql -m64 -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl
CFLAGS_COMMON = -Wno-unused-parameter -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../Administrateur -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++ -I Include/
CFLAGS_LIB_QT = /usr/lib64/libQt5Widgets.so /usr/lib64/libQt5Gui.so /usr/lib64/libQt5Core.so /usr/lib64/libGL.so 

CPP_SOURCES_GERANT := Source/Gerant/maingerant.cpp Source/Gerant/windowgerant.cpp Source/Gerant/moc_windowgerant.cpp
CPP_OBJECTS_GERANT := $(CPP_SOURCES_GERANT:%.cpp=%.o)

CPP_SOURCES_CLIENTS := Source/Client/mainclient.cpp Source/Client/windowclient.cpp Source/Client/moc_windowclient.cpp
CPP_OBJECTS_CLIENTS := $(CPP_SOURCES_CLIENTS:%.cpp=%.o)

all: Publicite Caddie AccesBD Serveur CreationBD Gerant Client

Publicite: Source/Publicite/Publicite.cpp 
	$(CC) $< -o $@ -I Include/

Caddie: Source/Caddie/Caddie.cpp
	$(CC) -g $< $(CFLAGSSQL) -o $@ -I Include/

AccesBD: Source/AccesBD/AccesBD.cpp
	$(CC) $< $(CFLAGSSQL) -o $@ -I Include/

Serveur: Source/Serveur/Serveur.cpp
	$(CC) $< -g $(CFLAGSSQL) Source/FichierClient/FichierClient.cpp -o $@ -I Include/

CreationBD: Source/CreationBD/CreationBD.cpp
	$(CC) $< $(CFLAGSSQL) -o $@ -I Include/

Gerant: $(CPP_OBJECTS_GERANT)
	$(CC) $(CPP_OBJECTS_GERANT) $(CFLAGS_LIB_QT) $(CFLAGSSQL) -o $@


Client:	$(CPP_OBJECTS_CLIENTS)
	$(CC) -g $(CPP_OBJECTS_CLIENTS) $(CFLAGS_LIB_QT) -o $@


%.o: %.cpp
	$(CC) $(CFLAGS_COMMON) $(CFLAGSSQL) -o $@ $<

clean:
	rm -rf Publicite Caddie AccesBD Serveur CreationBD Client Gerant
	find . -name "*.o" -type f -delete
