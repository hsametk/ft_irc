*IRC nedir?*
Internet Relay Chat
Gerçek zamanlı text tabanlı iletişim sistemidir.

Her kullanıcı bir client kullanır.
bu client bir servera bağlanır.
server mesajları diğer kullanıcılara iletir.
Merkezi bir yapıya sahip değildir.

| Konu     | Anlamı                     |
| -------- | -------------------------- |
| IRC      | text tabanlı chat protocol |
| Client   | komut gönderir             |
| Server   | karar verir ve dağıtır     |
| Channel  | chat room                  |
| User     | normal kullanıcı           |
| Operator | yetkili kullanıcı          |


Bir IRC sunucusunun temel görevleri şunlardır:

1. İstemcileri (Kullanıcıları) Birbirine Bağlamak
Sunucu, senin gibi bir "IRC Client" (istemci) kullanan kişilerin bağlantılarını kabul eder. Sen sunucuya bağlandığında, sunucu senin kimliğini (Nickname/Rumuz) doğrular ve seni ağdaki diğer kullanıcılarla görünür kılar.

2. Kanalları (Chat Odaları) Yönetmek
IRC'de iletişim kanallar üzerinden yürür. Sunucu şunları yapar:

Kullanıcı bir kanala girmek istediğinde (/join #kanal) onu o odaya dahil eder.

Kanaldaki mesaj trafiğini yönetir; yani sen bir mesaj yazdığında sunucu bu mesajı o an kanalda bulunan herkese anlık olarak iletir.

Kanal operatörlerini (moderatörleri) ve yetkileri takip eder.

3. Diğer Sunucularla Haberleşmek (Network)
Büyük IRC ağları (örneğin Libera.Chat veya QuakeNet) tek bir sunucudan oluşmaz. Dünya geneline yayılmış onlarca sunucu birbirine bağlıdır.

Sen Türkiye'deki bir sunucuya bağlıyken, Almanya'daki bir sunucuya bağlı olan arkadaşınla aynı kanalda yazışabilirsin.

Sunucular arka planda sürekli veri senkronizasyonu yaparak mesajların tüm ağa yayılmasını sağlar.

4. Servisleri Çalıştırmak
Çoğu modern IRC sunucusu, arka planda "Service" denilen yardımcı botlar çalıştırır:

NickServ: Rumuzunu şifreyle kaydetmeni sağlar.

ChanServ: Kanalların ayarlarını ve sahipliğini korur.

MemoServ: Çevrimdışı kullanıcılara kısa notlar bırakmanı sağlar.


*Client ↔ Server ilişkisi*

Burası ft_irc’nin kalbi ⚠️

📌 Basit akış:
Client (irssi)
    ↓ TCP
Server (senin yazacağın)
🔁 Süreç:

Client bağlanır

Server accept eder

Client komut gönderir

Server parse eder

Server cevap döner

🔥 Kritik fark:

HTTP:
👉 request → response → bitti

IRC:
👉 connection açık kalır (stateful)

*Channel Mantığı*
Server tarafında:

bir struct/class, içinde user listesi var

Örnek:
#42istanbul
 ├── samet
 ├── ali
 └── ayşe
Birisi mesaj atarsa:
PRIVMSG #42istanbul :selam
Server şunu yapar:
👉 listedeki herkese gönderir
*User vs Operator*
User (normal)
kanala girer
mesaj atar

👑 Operator (admin)
Ekstra yetkileri var:
KICK
INVITE
TOPIC
MODE
Örnek:
KICK #42 ali
👉 sadece operator 

*IRC Komut yapısı*
Örnek:

NICK samet
USER samet 0 * :realname
JOIN #42
PRIVMSG #42 :hello

Temel komut yapısı aşşağıdaki gibidir.
[KOMUT] [param1] [param2] ... [:son parametre]

İlk kelime komuttur diğerleri parametredir sonsuz parametre alınabilir.
Komutlar boşluklarla ayırlır son komut haricinde oda : ile başlar.
 #\ ise bir channel olduğu anlamına gelir.

 1. TCP (Bağlantı Temelli)
Telefon gibi: Önce bağlantı kurulur (Handshake), sonra veri akar.

Güvenilir: Veri kaybolmaz, sırası bozulmaz.

2. Stream-Based (Akış Temelli)
Su borusu gibi: TCP mesajın nerede bitip nerede başladığını bilmez. Sadece baytları sırayla akıtır.

3. Mesajlar Parça Parça mı Gelir?
Evet: Bir komut (NICK user\r\n) ağda parçalanabilir veya iki komut birleşip tek seferde gelebilir.

4. Çözüm: Buffer (Tampon)
Mantık: Gelen her veriyi bir std::string içinde biriktir.

Kural: İçeride \r\n görene kadar bekle, görünce o parçayı kesip "tam komut" olarak işle.

*Yani yapmamız gereken şey yukarıdaki kurallara göre parse etmemiz gerekiyor bir ilk olarak komutları alacağğız*
*sonrasında bunu bir buffer a atacağız \r\n görene kadar okuyacağız sonrasında onu işlemeye başlatacağız yukarıdaki gibi*
*ilk kelime komut son command : ile başlar gibi*

*What is the scoket*
*1. socket()*
The socket() is a system call in network programming that creates a new TCP socket in C++ that is defined inside the <sys/socket.h> header file.

Syntax
int sockfd = socket(AF_INET, SOCK_STREAM, 0);
Where,

int sockfd declares an integer variable that will store the socket file descriptor.
AF_INET indicates the socket will use the IPv4 address family.
SOCK_STREAM specifies that the socket will use TCP (a stream-oriented protocol) and,
0 lets the system choose the default protocol for the specified address family and socket type (which is TCP in this case).

2. bind()
The bind() method is associated with a socket, with a specific local address and port number which allows the socket to listen for incoming connection on that address.

Syntax
bind(sockfd, (struct sockaddr*)&address, sizeof(address));
Where,

sockfd is the file descriptor that represents the socket in your program and is used to perform various socket operations
(struct sockaddr)&address casts the address structure to a generic pointer type for the bind function.
sizeof(address) specifies the size of the address structure to inform the system how much data to expect.
3. listen()
The listen() function marks the socket as a passive socket which prepares a socket to accept incoming connection requests (for servers).

Syntax
listen(sockfd, 10);
Where,

sockfd is the file descriptor that represents the socket in your program and is used to perform various socket operations
10 is the backlog parameter, which specifies the maximum number of pending connections that can be queued while the server is busy.
4. accept()
The accept() function accepts a new connection from a client (for servers). It extracts the first connection request on the queue of pending connections and creates a new socket for that connection.

Syntax
int clientSocket = accept(sockfd, (struct sockaddr*)&clientAddress, &clientLen);
Where,

sockfd: It's the socket's file descriptor where it is used to perform various socket operations.
(struct sockaddr)&address: This is a type cast that converts the pointer type of clientAddress to a pointer of type struct sockaddr*.
&clientLen: It is a pointer to a variable that holds the size of the clientAddress.
C++ Client-Side Socket (Connecting to a Server)
The following methods are used for client-side communication:

1. connect()
This function is a system call that attempts to establish a connection to a specified server (for clients) using the socket.

Syntax
connect(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
Where,

sockfd is the file descriptor that represents the socket in your program and is used to perform various socket operations.
(struct sockaddr*)&serverAddress casts serverAddress to a struct sockaddr* pointer which enables compatibility with functions that require a generic socket address type.
sizeof(serverAddress) specifies the size of the serverAddress
2. send()
The send() function is a system call in socket programming which sends data to a connected socket.

Syntax
send(sockfd, "Hello", strlen("Hello"), 0);
Where,

sockfd is the file descriptor that represents the socket in your program and is used to perform various socket operations.
strlen("Hello") function returns the length of the string "Hello" (5 bytes), showing how many bytes of data to send.
0 lets the system choose the default protocol for the specified address family and socket type (which is TCP in this case).
3. recv()
The recv() function is a system call that is used to receive data from a connected socket which allows the client or server to read incoming messages.

Syntax
recv(sockfd, buffer, sizeof(buffer), 0);
Where,

sockfd is the file descriptor that represents the socket in your program and is used to perform various socket operations.
buffer is a pointer to the memory location where the received data will be stored. This buffer should be large enough to hold the incoming data.
sizeof(buffer) specifies the maximum number of bytes to read from the socket, which is typically the size of the buffer.
Closing the Client Socket
The close() method closes an open socket.

Syntax
close(sockfd);
Where,

close function is a system call that closes the file descriptor associated with the socket.
Required Header Files for Socket Programming
When programming with sockets in C or C++, specific header files must be included for the necessary declarations.

For Linux/Unix Systems
<sys/socket.h>
 <netinet/in.h>
 <arpa/inet.h>
 <unistd.h>
 <string.h>
 <errno.h>







 MUST (BUNLAR OLMADAN ft_irc OLMAZ)

Bunlar çekirdek server:

🔌 Socket + bağlantı

socket ✅

bind ✅

listen ✅

accept ✅

👉 server açmak için

📡 Veri iletişimi

send ✅

recv ✅

👉 mesaj alıp göndermek için

🔄 Multi-client (EN KRİTİK)

poll (veya select) 💥💥💥

👉 birden fazla client yönetmek için
👉 evaluator buraya bakar

⚙️ Socket ayarı

setsockopt ✅

👉 SO_REUSEADDR için (çok önemli)

🔢 Byte order

htons ✅

htonl 🔶 (genelde dolaylı kullanılır)

👉 port ayarlamak için

📁 FD yönetimi

close ✅

👉 leak olmaması için

🟧 HAVE TO (kullanman beklenir / pratikte gerekir)

Bunlar genelde projede kullanılır ama core değil:

⚙️ Non-blocking

fcntl 🔥

👉 O_NONBLOCK için
👉 poll ile birlikte kullanılır

🔁 Signal handling

signal veya sigaction 🔶

👉 ctrl+C yakalamak
👉 clean exit

🌐 Modern adres çözme (opsiyonel ama iyi)

getaddrinfo 🔶

freeaddrinfo 🔶

👉 özellikle client yazarsan

🔢 Byte dönüşüm (geri)

ntohs 🔶

ntohl 🔶

👉 debug / parsing

🌐 IP dönüşüm

inet_ntop 🔶

👉 log/debug için