# Sliding Windows Jarkom


## Compile Program

Gunakan comman `make` untuk meng-compile kedua recvfile dan sendfile

## Menjalankan Program

Buka 2 terminal, yang pertama bertindak sebagai sender dan satunya sebagai receiver.

Pada terminal sender, gunakan command `./sendfile <filename> <window_len> <buffer_size> <destination_ip> <destination_port>`.

Pada terminal receiver, gunakan command `./recvfile <filename> <window_size> <buffer_size> <port>`.

Setelah itu, program akan mengirimkan file yang dicantumkan pada argumen sendfile dan menuliskannya pada file yang dicantumkan pada receive file

# Cara Kerja Sliding Windows

### Sender :
1. Pertama, program akan membaca file yang ditulis pada argumen program sender
2. Lalu program akan memulai thread untuk menunggu ACK
3. Membuat Buffer, selama belum selesai maka lakukan:
	- Baca sebagian file dan masukkan ke buffer
	- Jika buffer telah terisi penuh, periksa apakah file telah selesai dibaca. Jika ya maka keluar dari loop
	- Jika tidak maka lanjutkan loop
4. Kirim buffer, selama pengiriman belum berakhir maka lakukan:
	- Periksa apakah window telah di ack, jika iya maka geser window
	- Kirim frame yang belum terkirim atau sudah time out
	- Ambil buffer selanjutnya dan kirimkan seperti langkah ini jika semua frame pada buffer saat ini telah di ACK
5. Pengiriman selesai

### Receiver :
1. Menunggu transmisi datang
2. Jika transmisi diterima, maka terima frame-frame yang datang
3. Selama menerima frame belum selesai lakukan:
	- Jika frame mengandung error maka kirim NAK (Negative Acknowledgement)
	- Jika tidak maka kirim ACK kepada pengirim
	- Set max sequence menjadi sequence of frame ketika EOT (End of Transmission)
	- Pindah ke buffer selanjutnya jika semua frame pada buffer telah diterima

# Pembagian Tugas
1. 13516008 - Muhammad Aufa Helfiandri : createFrame, createACK, readFrame, readACK
2. 13516056 - Muhammad Rafli Al Khadafi : sendACK, recvfile
3. 13516125 - Aldo Azali : listenACK, sendfile
