#!/usr/bin/python

import logging
import threading
import thread
import time
import serial

#logging.getLogger('socketIO-client').setLevel(logging.DEBUG)
logging.basicConfig()

from socketIO_client import SocketIO, LoggingNamespace

IDLE = 0
MUSIC = 1
RADIO = 2

status = IDLE
playing = False
stations = []
songs = []
    
currStation = 0
currSong = 0
lastStation = 0
lastSong = 0

def on_connect():
    print('connect')

def on_disconnect():
    print('disconnect')

def on_pushState(data):
    global lastSong, lastStation
    print('STATE:', data)
    try:
        pos = data['position']
        service = data['service']
        if service == 'mpd':
            print ('LAST SONG = %i' % pos)
            lastSong = pos
    except:
        pass

def browseSources(data):
    for src in data:
        print (src['name'],src['uri'])

def addLibraryItem(data):
    global songs
    nav = data['navigation']
    lists = nav['lists']
    for l in lists:
        for i in l['items']:
            service = i['service']
            if service == 'mpd':
                print ('Add song: %s [%s]' % (i['title'], i['uri']))
                songs.append((i['title'], i['uri']))
            elif service == 'webradio':
                print ('Add station: %s [%s]' % (i['title'], i['uri']))
                stations.append((i['title'], i['uri']))                
            
def serialReader(port):
    buf = ''
    while True:
        c = port.read(1)
        if c == '\n':
            yield buf
            buf = ''
        else:
            buf += c
            
def updateLibrary(uri = None):
    stations = []
    songs = []
    socket.emit('browseLibrary', {'uri': 'radio/myWebRadio'})
    socket.emit('browseLibrary', {'uri': 'favourites'})
    socket.wait(seconds=1)
            
serial = serial.Serial('/dev/pts/7')
reader = serialReader(serial)
    
with SocketIO('http://volumio.local', 3000, LoggingNamespace) as socket:
    socket.on('connect', on_connect)
    socket.on('disconnect', on_disconnect)
    socket.on('pushState', on_pushState)
    socket.on('pushBrowseSources', browseSources)
    socket.on('pushBrowseLibrary', addLibraryItem)

    def setShuffle(shuffle):
        socket.emit('setRandom', {'value': shuffle})        
    
    def playRadio(stationIdx):
        global stations, playing
        station = stations[stationIdx]
        print('Play radio %s' % station[0])
        socket.emit('replaceAndPlay', {'uri': station[1], 'title':station[0], 'service':'webradio'})
        playing = True

    def playSong(songIdx):
        global songs
        song = songs[songIdx]
        print('Play song %s' % song[0])
        socket.emit('replaceAndPlay', {'uri': song[1], 'title':song[0], 'service':'mpd'})

    def addSong(songIdx):
        global songs
        song = songs[songIdx]
        print('Add song %s' % song[0])
        socket.emit('addToQueue', {'uri': song[1], 'title':song[0], 'service':'mpd'})

    updateLibrary()
    setShuffle(False)
    socket.emit('getState')
    socket.wait(seconds=1)
        
    while True:
        cmd = reader.next()
        if cmd == '+':
            print('VOL+')
            socket.emit('volume','+')
        elif cmd == '-':
            print('VOL-')
            socket.emit('volume','-')
        elif cmd == 'R':
            if status <> RADIO:
                currStation = lastStation
                playRadio(currStation)
                socket.wait(seconds=1)
                status = RADIO
            elif playing:
                socket.emit('stop')
                socket.wait(seconds = 1)
                playing = False
            else:
                socket.emit('play')
                socket.wait(seconds = 1)
                playing = True
        elif cmd == 'R+':
            if status == RADIO:
                currStation += 1
                if currStation >= len(stations):
                    currStation = 0
                lastStation = currStation
                playRadio(currStation)
        elif cmd == 'R-':
            if status == RADIO:
                currStation -= 1
                if currStation < 0:
                    currStation = len(stations) - 1
                lastStation = currStation
                playRadio(currStation)
        elif cmd == 'M':
            if status <> MUSIC:
                currSong = 0
                setShuffle(False)
                playSong(0)
                for idx in range(1, len(songs)):
                    addSong(idx)
                socket.wait(seconds = 1)
                status = MUSIC
                playing = True
            elif playing:
                socket.emit('stop')
                socket.wait(seconds = 1)
                playing = False
            else:
                socket.emit('play')
                socket.wait(seconds = 1)
                playing = True                               
        elif cmd == 'M+':
            if status == MUSIC:
                socket.emit('next')
        elif cmd == 'M-':
            if status == MUSIC:
                socket.emit('previous')
        elif cmd == 'P':
            print('PLAY')
            socket.emit('play')
            socket.wait(seconds = 1)
        elif cmd == 'S':
            print('STOP')
            socket.emit('stop')
            socket.wait(seconds = 1)
        elif cmd == 'X':            
            print('SHUFFLE')
            setShuffle(True)
            socket.wait(seconds=1)
            socket.emit('next')




        

#    socket.emit('getState')
 #   socket.emit('getBrowseSources')
    #socket.wait(seconds=1)
    #socket.emit('browseLibrary', {'uri': 'radio'})
