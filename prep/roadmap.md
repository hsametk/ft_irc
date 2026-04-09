ft_irc feature roadmap
0. Temel hazırlık

Önce ekipçe bunları netleştir:

./ircserv <port> <password> akışı
hangi class’lar var
hangi class neyi tutuyor
Mac’te geliştirip Ubuntu’da test edileceği için sadece POSIX uyumlu şeyler kullanma
başta gereksiz bonuslara girmeme
Minimum class fikri
Server
Client
Channel
Server ne tutsun?
server socket fd
pollfd listesi
client container’ı
channel container’ı
server password
Client ne tutsun?
fd
nickname
username
realname
register durumu
input buffer
girdiği channel’lar hakkında bilgi
Channel ne tutsun?
kanal adı
topic
user listesi
operator listesi
mode bilgileri
invite listesi
key / user limit
1. Socket setup feature

İlk feature sadece server’ın ayağa kalkması olsun.

Hedef
socket aç
bind yap
listen yap
poll ile bekle
yeni bağlantıyı accept et
Demo kriteri
irssi bağlanınca server terminalinde
“new client connected”
fd bilgisi
görünmeli
Bu feature bitmeden geçme

Şunları net bil:

accept ne döndürür
recv == 0 ne demek
non-blocking ister misin
poll event’lerini nasıl yöneteceksin
2. Client connection management feature

Yeni bağlanan client’ı sadece kabul etmek yetmez, onu yönetebilmen lazım.

Hedef
her yeni client için Client objesi oluştur
fd ile client objesini eşle
disconnect olunca temizle
pollfd listesinden çıkar
Demo kriteri
2 client bağlanabilsin
biri çıkınca server çökmesin
kalan client çalışmaya devam etsin
3. Input buffer feature

Burası çok kritik.

IRC, stream-based çalışır. Komutlar parçalı gelebilir.

Hedef

Her client için:

gelen veriyi buffer’a ekle
\r\n görünce tam satır çıkar
tek recv içinde birden fazla komut gelirse ayır
parçalı gelen komutu sonraki recv ile tamamla
Demo kriteri

Server logunda şunlar düzgün ayrılsın:

CAP LS 302
PASS ...
NICK ...
USER ...
4. Message parsing feature

Artık raw string değil, gerçek IRC komutu parse etmen lazım.

Hedef

Bir satırı şuna ayır:

command
params
trailing parametre

Örnek:
USER samet 0 * :real name

Burada:

command = USER
params = samet, 0, *
trailing = real name
Demo kriteri

Her gelen satır için log:

command ne
kaç parametre var
trailing var mı
5. Registration feature

IRC’de user direkt konuşamaz. Önce register olmalı.

Hedef

Şunları handle et:

PASS
NICK
USER
Düşünmen gereken state

Client için flag mantığı:

pass_ok
nick_set
user_set
registered
Demo kriteri

Bu üçü tamamlanınca:

client “registered” olsun
server bir welcome akışı başlatsın
6. CAP handling feature

irssi ile çalışmak için bu kısmı en azından minimum handle etmen gerekecek.

Hedef
CAP LS 302 gelince client’ı beklemede bırakmamak
minimum uygun cevapla handshake’in devam etmesini sağlamak
Demo kriteri

irssi’de:

“Waiting for CAP LS response...” takılmaması
sonra NICK/USER aşamasına geçebilmesi
7. Command dispatcher feature

Parse ettiğin command’i doğru handler’a yönlendir.

Hedef

Mesela:

PASS → handlePass
NICK → handleNick
USER → handleUser
JOIN → handleJoin
Demo kriteri

Kodun tek yerde dağılmaması.
if else cehennemine dönmemesi.

8. Basic reply / send feature

Artık server sadece okumasın, cevap da versin.

Hedef
client’a formatlı IRC satırı gönderebil
tüm gönderilen mesajları tek helper ile üret
\r\n unutma
Demo kriteri

Server loglasın:

ne gönderdi
kime gönderdi

Bu feature olmadan irssi’de düzgün görüntü alamazsın.

9. Channel creation and JOIN feature

Artık gerçek IRC hissi burada başlar.

Hedef
JOIN #kanal
kanal yoksa oluştur
varsa kullanıcıyı ekle
join mesajını yayınla
Düşün

Kanalı kim oluşturuyor?
Cevap: server

Demo kriteri

2 client aynı kanala girebilsin.

10. Channel membership feature

Client hangi channel’da, channel içinde kim var, bunları düzgün tut.

Hedef
channel user listesi
client’ın joined channel bilgisi
kanaldan çıkınca iki taraftan da sil
Demo kriteri

Memory/state bozulmadan:

join
part
quit
çalışsın
11. PRIVMSG feature

Bence en gösterişli demo feature budur.

Hedef
user → user mesajı
user → channel mesajı
Kontroller
hedef user var mı
hedef channel var mı
gönderen kanalda mı
Demo kriteri

İki irssi aç:

ikisi de #42’ye girsin
birinden yazınca diğerinde görünsün

Yarın arkadaşına göstermek için en güzel aşama bu olur.

12. PART and QUIT feature

Temizlik aşaması.

Hedef
PART
QUIT
bağlantı kopunca otomatik cleanup
Demo kriteri

User çıkınca:

channel’dan silinsin
diğerlerine bildirilsin
boş channel silinsin mi, karar verin
13. Channel operator feature

Bundan sonra kanal yetkileri.

Hedef

Kanal operator mantığını oturt:

ilk giren op mu olacak
op listesi nasıl tutulacak

Bu olmadan KICK, INVITE, MODE sağlıklı olmaz.

14. TOPIC feature
Hedef
topic görüntüleme
topic değiştirme
Sonra ek kontrol
+t varsa sadece operator değiştirebilsin
15. INVITE feature
Hedef
invite-only kanal için kullanıcı davet etme
invite listesi tutma
16. KICK feature
Hedef
sadece operator kick atabilsin
user kanaldan çıkarılsın
gerekli broadcast yapılsın
17. MODE feature

En dikkat isteyen feature’lardan biri.

Subjectte geçen temel channel mode’lar
i invite-only
t topic only ops
k channel key
o give/take operator
l user limit
Hedef

Önce bunları tek tek destekle.
Hepsini aynı anda çözmeye çalışma.

İlerleme sırası önerim
+i
+t
+k
+l
+o
18. Error handling feature

Bu çok önemli ama çoğu kişi sona bırakıyor.

Hedef

Eksik parametre, yetkisiz işlem, olmayan nick, olmayan channel gibi durumlarda düzgün cevap dönmek.

Demo kriteri

Server çökmesin.
Yanlış komutta bile kontrollü davransın.

19. Cleanup and robustness feature
Hedef
client disconnect
kanal boşalması
poll listesinden temiz çıkış
fd leak olmaması
map/vector iterator hatası olmaması
20. Cross-platform test feature

Bu proje için özellikle önemli.

Mac + Ubuntu için dikkat
Linux-specific saçma bağımlılıklar ekleme
header include’larını temiz tut
poll, socket, fcntl, unistd gibi standart POSIX kullan
derleyici farklarında warning çıkıyor mu bak
Test et
Mac’te compile
Ubuntu’da compile
irssi ile bağlan
aynı senaryoları iki ortamda dene