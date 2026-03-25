Süper, artık en kritik kısımdayız: doğru başlangıç + doğru ekip planı 🔥
2 kişi olmanız avantaj ama yanlış ilerlerseniz çakışırsınız.

Ben sana 42 uyumlu, ekipli, adım adım net plan veriyorum.

👉 Bu proje çok bağımlı parçalar içerir

❗ En doğrusu: core’u birlikte yazmak, feature’ları bölmek

🧱 1. İlk gün: sadece ARCHITECTURE (kod yazma!)

İkiniz birlikte şunu netleştirin:

🔹 Hangi class’lar olacak?

Minimum:

Server
Client
Channel
🔹 Server ne tutacak?

Düşün:

server socket fd

pollfd listesi

client listesi

channel listesi

🔹 Client ne tutacak?

fd

nickname

username

registered mı?

buffer (ÇOK ÖNEMLİ)

🔹 Channel ne tutacak?

channel name

user listesi

operator listesi

👉 Bunu kağıtta netleştirmeden KOD YAZMAYIN

🏗️ 2. Gün: CORE LOOP (birlikte yazın)

İkiniz beraber yazacaksınız:

🎯 hedef:
while (true)
{
    poll()

    if (new connection)
        accept()

    if (client data)
        recv()
}
🔥 Bu aşamada:

✔ multi-client çalışmalı
✔ crash olmamalı
✔ disconnect yakalanmalı

❌ command yok
❌ IRC logic yok

🧪 3. Gün: BUFFER sistemi (yine birlikte)

Bu aşama çok kritik:

👉 Her client için:

client.buffer += recv_data;

Sonra:

while (buffer içinde \r\n varsa)
    komutu çıkar

👉 Bu çözülmeden ilerlemeyin

🧩 4. Gün: Burada bölünebilirsiniz (İLK BÖLÜM)

Artık sistem stabil → şimdi iş bölümü yapılır

👨‍💻 Kişi 1: NETWORK / CORE

poll loop

client yönetimi

recv/send

buffer sistemi

👨‍💻 Kişi 2: IRC LOGIC

command parsing

NICK / USER / PASS

command handler

👉 Ama dikkat:

HER GÜN merge yapın
ayrı ayrı kopmayın

🧠 5. GÜN: REGISTRATION

İlk gerçek IRC kısmı:

PASS

NICK

USER

👉 Burada şu state’i kur:

client.registered = true/false
🧪 6. GÜN: irssi ile test

Artık:

/connect localhost 6667

dediğinde:

👉 server çökmemeli
👉 komutları loglamalı

💬 7. GÜN: BASİT KOMUTLAR

Başlangıç:

PING → PONG

QUIT

🧱 8. GÜN: CHANNEL

JOIN

PART

💥 9. GÜN: MESAJ

PRIVMSG

👑 10. GÜN: OPERATOR

KICK

INVITE

MODE

🔥 ÇOK KRİTİK TAKIM KURALI

Her gün:

birlikte 1 saat design konuşun

kod yazın

test edin (irssi ile)

commit + merge

🧪 irssi’yi nasıl kullanacaksınız?

Irssi sizin:

🧪 “gerçek kullanıcı simülatörünüz”

Test akışı:
Terminal 1:
./ircserv 6667
Terminal 2:
irssi
/connect localhost 6667 samet
Sonra gözlemle:

hangi komutlar geliyor?

sırayla mı geliyor?

eksik mi geliyor?

🎯 Sana en kritik tavsiye

Şu hatayı yapma:

❌ “JOIN yazayım artık”
❌ “PRIVMSG yapalım”

👉 Önce:

recv + buffer + poll = %100 sağlam

🧠 Sana challenge (çok önemli)

Şunu düşün:

👉 1 client:

NICK samet\r\nUSER samet\r\n

👉 2 parçaya bölünürse:

NICK sa
met\r\nUSER samet\r\n

Bunu nasıl handle edeceksin?