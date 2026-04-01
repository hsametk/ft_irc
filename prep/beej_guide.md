# What is a socket?
Socket, programların birbirleriyle iletişim kurmasını sağlayan bir yapıdır.
Unix’te her şey bir dosya (file) gibi davranır.
Bu yüzden iletişim, file descriptor (fd) adı verilen bir sayı üzerinden yapılır.
Socket de aslında bir file descriptor’dır.
Bu descriptor, socket() fonksiyonu ile oluşturulur.
Socket üzerinden veri gönderip almak için genelde send() ve recv() kullanılır.
İstersen read() ve write() da kullanabilirsin ama send/recv daha fazla kontrol sağlar.
Farklı socket türleri vardır ama en yaygın olanı Internet (TCP/IP) socket’leridir.

## Socket Türleri
İki ana tür vardır:
Stream Socket (SOCK_STREAM)
Datagram Socket (SOCK_DGRAM)
🔹 Stream Socket (TCP)
Bağlantılıdır (connection-oriented)
Güvenilirdir (reliable)
Veriler sıralı gelir (order korunur)
Veri hatasız ulaşır
TCP protokolünü kullanır

### Kullanım:

SSH, Telnet
Web (HTTP)

👉 Özet:

“Ne gönderirsen, aynı sırayla ve eksiksiz karşıya gider”

🔹 Datagram Socket (UDP)
Bağlantısızdır (connectionless)
Güvenilir değildir
Paket:
Kaybolabilir
Sırası değişebilir
Ama gelen veri bozulmaz (packet içi doğru gelir)
UDP protokolünü kullanır

📌 Kullanım:

Oyunlar
Video / ses streaming
DHCP, TFTP

👉 Özet:

“Gönder ve unut (fire-and-forget)”

🔹 TCP vs UDP farkı (çok kritik)
TCP → güvenli ama yavaş
UDP → hızlı ama garanti yok
 
🔹 Önemli mantık
UDP’de güvenilirlik gerekiyorsa:
Uygulama kendi ACK (onay) sistemi kurar
🔥 En kritik cümle (ezberlik)
TCP = doğru + sıralı + garanti
UDP = hızlı + garanti yok

### 2.2 Low level Nonsense and Network Theory

Ağ Teorisi ve Veri Kapsülleme Özeti
1. Veri Kapsülleme (Data Encapsulation) Nedir?
Bir veri paketinin ağ üzerinden gönderilirken her katmanda yeni bir "zarfın" içine konulması işlemidir.

Süreç: Veri önce uygulama protokolü (örn: TFTP) ile paketlenir, sonra sırasıyla UDP, IP ve en son fiziksel katman (örn: Ethernet) başlıkları (headers) eklenir.

Tersi (Decapsulation): Alıcı bilgisayar paketi aldığında bu başlıkları katman katman soyar. En sonunda sadece saf veri kalır.

2. Katmanlı Ağ Modelleri
Metinde iki farklı modelden bahsediliyor:

ISO/OSI Modeli (7 Katmanlı): Akademik ve genel standarttır.

(Uygulama, Sunum, Oturum, Taşıma, Ağ, Veri Bağı, Fiziksel)

Unix/TCP-IP Modeli (4 Katmanlı): Pratik kullanıma daha uygundur.

Application (Uygulama): Kullanıcının etkileşime girdiği yer (Telnet, FTP).

Transport (Taşıma): Verinin nasıl gideceğini belirler (TCP, UDP).

Internet (İnternet): Yönlendirme ve IP katmanı.

Network Access (Ağ Erişimi): Donanım ve fiziksel bağlantı (Wi-Fi, Ethernet).

3. Programcı İçin Kolaylığı
Soket programlamanın en büyük avantajı bu katmanların şeffaf (transparent) olmasıdır. Sen bir program yazarken verinin kabloyla mı yoksa Wi-Fi ile mi gittiğini düşünmek zorunda kalmazsın.

Stream Sockets (TCP): Sadece send() fonksiyonuyla veriyi gönderirsin.

Datagram Sockets (UDP): sendto() kullanırsın.

Geri Kalan İşler: Taşıma ve İnternet katmanı başlıklarını Kernel (Çekirdek) oluşturur; donanım ise en dıştaki ağ erişim başlığını ekler. Sen sadece verine odaklanırsın.

Not: Yazarın da belirttiği gibi, yönlendirme (routing) gibi karmaşık detayları genellikle işletim sistemi ve donanım hallettiği için bir soket programcısı olarak bunlara derinlemesine boğulmana gerek kalmıyor. 

# Byte Order (Endianness) Özeti
Bilgisayarların veriyi (özellikle sayıları) hafızada saklama biçimleri farklılık gösterir. Metin bunu iki ana gruba ayırıyor:

1. Big-Endian (Network Byte Order)
Mantık: Sayı olduğu gibi, yani en anlamlı bayt başa gelecek şekilde saklanır.

Örnek: 0xb34f sayısı hafızada sırasıyla b3 ve 4f olarak tutulur.

Önemi: İnternet dünyası bu sıralamayı standart kabul etmiştir. Bu yüzden buna Network Byte Order denir.

2. Little-Endian (Host Byte Order)
Mantık: Sayı tersinden, yani en küçük bayt başa gelecek şekilde saklanır.

Örnek: 0xb34f sayısı hafızada sırasıyla 4f ve b3 olarak tutulur.

Kim Kullanır? Intel ve Intel uyumlu (x86) işlemciler bu yöntemi kullanır. Bilgisayarınızın kendi içindeki bu sıralamasına Host Byte Order denir.

Neden Önemli?
Eğer senin bilgisayarın (Little-Endian) ağ üzerinden bir sayı gönderirse ve karşıdaki bilgisayar (Big-Endian) bunu kendi formatında okumaya çalışırsa, sayı tamamen yanlış anlaşılır. Bu yüzden veriyi ağa göndermeden önce "standart dile" çevirmelisin.

Dönüşüm Fonksiyonları
Bu karmaşayı çözmek için her seferinde işlemci mimarini kontrol etmene gerek yok. C dilinde bu işi yapan standart fonksiyonlar vardır:

| 🛠️ Fonksiyon | 💡 Açılımı (Mnemonic) | 📝 Açıklama | 🔄 Yön |
| :--- | :--- | :--- | :---: |
| `htons()` | **H**ost **T**o **N**etwork **S**hort | 16-bit Port numaralarını çevirir. | ⬆️ Giden |
| `htonl()` | **H**ost **T**o **N**etwork **L**ong | 32-bit IP adreslerini çevirir. | ⬆️ Giden |
| `ntohs()` | **N**etwork **T**o **H**ost **S**hort | Alınan Port bilgisini makineye çevirir. | ⬇️ Gelen |
| `ntohl()` | **N**etwork **T**o **H**ost **L**ong | Alınan IP bilgisini makineye çevirir. | ⬇️ Gelen |

> **Not:** Intel işlemcili bir bilgisayar (Little-Endian) kullanıyorsanız, `htons()` fonksiyonu baytların yerini değiştirir. Eğer sisteminiz zaten Big-Endian ise, fonksiyon hiçbir şey yapmadan değeri döndürür. Bu sayede kodunuz **taşınabilir (portable)** olur.

# Soket Programlama Veri Yapıları (Structs) Özeti
1. Socket Descriptor (Soket Tanımlayıcı)
En kolayı budur. Bir soket oluşturduğunda işletim sistemi sana bir int döndürür. Bu tam sayı, o bağlantıyı temsil eden bir ID gibidir.

2. struct addrinfo (Modern Yaklaşım)
Bu yapı, bağlantı kurmadan önce hazırlık yapmak için kullanılır. En büyük avantajı IP versiyonundan bağımsız (agnostic) olmasıdır; yani kodun hem IPv4 hem de IPv6 ile çalışabilir.

Önemli Alanlar:

ai_family: AF_INET (IPv4), AF_INET6 (IPv6) veya AF_UNSPEC (Her ikisi de olur).

ai_socktype: SOCK_STREAM (TCP) veya SOCK_DGRAM (UDP).

ai_addr: Gerçek adres bilgilerini (IP ve Port) içeren başka bir struct'a (sockaddr) işarettir (pointer).

ai_next: Bu yapı bir bağlı liste (linked list) şeklindedir. getaddrinfo() fonksiyonunu çağırdığında sana birden fazla sonuç dönebilir, bu işaretçiyle bir sonrakine geçersin.

Gemini şunu dedi:
Beej's Guide'ın bu bölümü, ağ programlamanın "mutfağına" girdiğimiz yerdir. İşin içine veri yapıları (structlar) giriyor. ft_irc projesinde en çok haşır neşir olacağın yapıları senin için özetledim:

🏗️ Soket Programlama Veri Yapıları (Structs) Özeti
1. Socket Descriptor (Soket Tanımlayıcı)
En kolayı budur. Bir soket oluşturduğunda işletim sistemi sana bir int döndürür. Bu tam sayı, o bağlantıyı temsil eden bir ID gibidir.

2. struct addrinfo (Modern Yaklaşım)
Bu yapı, bağlantı kurmadan önce hazırlık yapmak için kullanılır. En büyük avantajı IP versiyonundan bağımsız (agnostic) olmasıdır; yani kodun hem IPv4 hem de IPv6 ile çalışabilir.

Önemli Alanlar:

ai_family: AF_INET (IPv4), AF_INET6 (IPv6) veya AF_UNSPEC (Her ikisi de olur).

ai_socktype: SOCK_STREAM (TCP) veya SOCK_DGRAM (UDP).

ai_addr: Gerçek adres bilgilerini (IP ve Port) içeren başka bir struct'a (sockaddr) işarettir (pointer).

ai_next: Bu yapı bir bağlı liste (linked list) şeklindedir. getaddrinfo() fonksiyonunu çağırdığında sana birden fazla sonuç dönebilir, bu işaretçiyle bir sonrakine geçersin.

Hazır .md Notu (Dökümanına Ekleyebilirsin)
Markdown
## 🧱 Temel Veri Yapıları (Structs)

Ağ programlamada verileri düzenli tutmak için belirli yapı taşlarını kullanırız.

### 1. Socket Descriptor
Sistemde her soket bir tam sayı ile temsil edilir.
```cpp
int sockfd; // Standart bir tam sayıdır.
2. struct addrinfo
Modern ağ programlamanın temelidir. getaddrinfo() fonksiyonu ile doldurulur.

Alan	Tip	Açıklama
ai_family	int	AF_INET (IPv4) veya AF_INET6 (IPv6)
ai_socktype	int	SOCK_STREAM (TCP) veya SOCK_DGRAM (UDP)
ai_addr	struct sockaddr*	IP ve Port bilgisini tutan yapıya pointer
ai_next	struct addrinfo*	Bağlı listedeki bir sonraki sonuç
💡 İpucu: ft_irc projesinde AF_INET ve SOCK_STREAM (TCP) kombinasyonunu kullanacaksın.

3. Neden getaddrinfo()?
Eskiden bu yapıları (IP, Port, Aile) elle doldururduk. Ancak getaddrinfo():

Alanları otomatik doldurur.

Hem IPv4 hem IPv6 destekler.

DNS sorgularını (hostname lookup) senin yerine yapar.
```
