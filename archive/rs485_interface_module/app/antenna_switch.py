from flask import Flask, render_template, request
from flask_socketio import SocketIO, emit, send, join_room
import serial

app = Flask(__name__)
socketio = SocketIO(app)

ser = serial.Serial('/dev/ttyUSB0', 9600)

ANTENNAS = [ 'HexBeam 20m, 15m 10m', 'Vertical 160m, 80m, 40m', 'Dipol 80m, 40m, 20m', 'Long Wire', 'Antena 5', 'Antena 6']

@app.route('/')
def index():
  return render_template('index.html', ANTENNAS = ANTENNAS)

@socketio.on('antenna_switch')
def on_switch(radio, antenna):
  print("Selected antenna", antenna, "for radio", radio)
  cmd = 'SET {} {}\n'.format(radio, antenna)
  ser.write(cmd.encode())
  result = ser.readline().decode('utf-8').rstrip()
  emit('status', result)
  # emit('status', '!BUSY')

  ser.write(b'GET 1\n')
  r1 = ser.readline().decode('utf-8').rstrip()
  ser.write(b'GET 2\n')
  r2 = ser.readline().decode('utf-8').rstrip()

  msg = '"radio1": {}, "radio2": {}'.format(r1, r2)
  # msg = '"radio1": {}, "radio2": {}'.format(antenna, 0)
  emit('antenna', '{'+msg+'}', broadcast=True)


if __name__ == '__main__':
  socketio.run(app, debug=True, host='0.0.0.0')