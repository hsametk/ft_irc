#include "../include/Server.hpp"
#include "../include/CmdHelpers.hpp"

// ---------------------------------------------------------------------------
// Yardımcı: string'in geçerli pozitif sayı olup olmadığını kontrol eder
// ---------------------------------------------------------------------------
static bool isPositiveNumber(const std::string &s)
{
    if (s.empty())
        return false;
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (!std::isdigit(s[i]))
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// +i / -i  — Invite-only
// ---------------------------------------------------------------------------
bool Server::applyModeI(Client&, Channel &ch, bool adding,
                        const std::string&, const std::vector<std::string>&, size_t&,
                        std::string &appliedModes, std::string &)
{
    ch.setInviteOnly(adding);
    if (adding)
        appliedModes += "+i";
    else
        appliedModes += "-i";
    return true;
}

// ---------------------------------------------------------------------------
// +t / -t  — Topic sadece operatörler değiştirebilir
// ---------------------------------------------------------------------------
bool Server::applyModeT(Client&, Channel &ch, bool adding,
                        const std::string&, const std::vector<std::string>&, size_t&,
                        std::string &appliedModes, std::string &)
{
    ch.setTopicOpOnly(adding);
    if (adding)
        appliedModes += "+t";
    else
        appliedModes += "-t";
    return true;
}

// ---------------------------------------------------------------------------
// +k / -k  — Kanal şifresi
// ---------------------------------------------------------------------------
bool Server::applyModeK(Client &sender, Channel &ch, bool adding,
                        const std::string &channelName,
                        const std::vector<std::string> &modeParams, size_t &paramIdx,
                        std::string &appliedModes, std::string &appliedParams)
{
    if (adding)
    {
        // +k parametre gerektirir
        if (paramIdx >= modeParams.size() || modeParams[paramIdx].empty())
        {
            sendError(sender, ERR_NEEDMOREPARAMS, channelName + " +k :Not enough parameters");
            return false;
        }
        ch.setKey(modeParams[paramIdx]);
        appliedModes  += "+k";
        appliedParams += " " + modeParams[paramIdx];
        ++paramIdx;
    }
    else
    {
        // -k parametre almaz; şifreyi temizle
        ch.setKey("");
        appliedModes += "-k";
    }
    return true;
}

// ---------------------------------------------------------------------------
// +l / -l  — Üye limiti
// ---------------------------------------------------------------------------
bool Server::applyModeL(Client &sender, Channel &ch, bool adding,
                        const std::string &channelName,
                        const std::vector<std::string> &modeParams, size_t &paramIdx,
                        std::string &appliedModes, std::string &appliedParams)
{
    if (adding)
    {
        // +l parametre gerektirir ve pozitif sayı olmalı
        if (paramIdx >= modeParams.size() || !isPositiveNumber(modeParams[paramIdx]))
        {
            sendError(sender, ERR_NEEDMOREPARAMS, channelName + " +l :Invalid or missing limit");
            return false;
        }
        ch.setLimit(std::atoi(modeParams[paramIdx].c_str()));
        appliedModes  += "+l";
        appliedParams += " " + modeParams[paramIdx];
        ++paramIdx;
    }
    else
    {
        // -l → limiti kaldır
        ch.setLimit(0);
        appliedModes += "-l";
    }
    return true;
}

// ---------------------------------------------------------------------------
// +o / -o  — Operatör yetkisi ver / al
// ---------------------------------------------------------------------------
bool Server::applyModeO(Client &sender, Channel &ch, bool adding,
                        const std::string &channelName,
                        const std::vector<std::string> &modeParams, size_t &paramIdx,
                        std::string &appliedModes, std::string &appliedParams)
{
    if (paramIdx >= modeParams.size() || modeParams[paramIdx].empty())
    {
        sendError(sender, ERR_NEEDMOREPARAMS, channelName + " +o/-o :Not enough parameters");
        return false;
    }
    const std::string &targetNick = modeParams[paramIdx];
    ++paramIdx;

    // 7. Hedef nick var mı?
    Client *target = CmdHelpers_getClientByNick(*this, sender, targetNick);
    if (!target)
        return false;

    // 8. Hedef kanalda mı?
    if (!CmdHelpers_requireTargetMember(*this, sender, ch, *target))
        return false;

    std::string prefix = ":" + sender.getNickname() + "!" +
                         sender.getUsername() + "@localhost";

    if (adding)
    {
        ch.addOperator(target->getFd());
        appliedModes  += "+o";
        appliedParams += " " + targetNick;
        // Kanaldaki herkese MODE bildir (irssi @ prefix günceller)
        std::string modeMsg = prefix + " MODE " + channelName + " +o " + targetNick + "\r\n";
        ch.broadcast(modeMsg);
    }
    else
    {
        ch.removeOperator(target->getFd());
        appliedModes  += "-o";
        appliedParams += " " + targetNick;
        std::string modeMsg = prefix + " MODE " + channelName + " -o " + targetNick + "\r\n";
        ch.broadcast(modeMsg);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Server::handleMode
//
// Format  : MODE #channel <modestring> [param1] [param2] ...
// Örnekler: MODE #room +i
//           MODE #room +k secret
//           MODE #room +l 10
//           MODE #room +o nick
//           MODE #room -o nick
//           MODE #room +itk secret   ← çoklu mod
//
// Kontrol sırası (subject + RFC 1459):
//   1. Parametre eksik?       → ERR_NEEDMOREPARAMS (461)   [Auth.cpp'de]
//   2. Kanal var mı?          → ERR_NOSUCHCHANNEL  (403)
//   3. Sender kanalda mı?     → ERR_NOTONCHANNEL   (442)
//   4. Sender operator mü?    → ERR_CHANOPRIVSNEEDED (482)
//   -- her mod karakteri için --
//   5. +k eksik param?        → ERR_NEEDMOREPARAMS (461)
//   6. +l geçersiz sayı?      → ERR_NEEDMOREPARAMS (461)
//   7. +o/-o: nick var mı?    → ERR_NOSUCHNICK     (401)
//   8. +o/-o: target kanalda? → ERR_NOTONCHANNEL   (442)
// ---------------------------------------------------------------------------
void Server::handleMode(Client &sender, const std::string &channelName,
                        const std::string &modeStr,
                        const std::vector<std::string> &modeParams)
{
    // 2. Kanal var mı?
    Channel *ch = CmdHelpers_getChannelOrError(*this, sender, channelName);
    if (!ch)
        return;
    // 3. Sender kanalda mı?
    if (!CmdHelpers_requireMember(*this, sender, *ch))
        return;
    // 4. Sender operator mü?
    if (!CmdHelpers_requireOperator(*this, sender, *ch))
        return;
    // Uygulanan mod değişikliklerini RPL_CHANNELMODEIS (324) için biriktir
    std::string appliedModes;
    std::string appliedParams;
    bool        adding   = true;  // '+' → true, '-' → false
    size_t      paramIdx = 0;     // modeParams içindeki sıradaki parametre

    for (size_t i = 0; i < modeStr.size(); ++i)
    {
        char c = modeStr[i];
        if (c == '+') { adding = true; continue; }
        if (c == '-') { adding = false; continue; }

        bool ok = true;
        if (c == 'i')
        {
            ok = applyModeI(sender, *ch, adding, channelName, modeParams, paramIdx, appliedModes, appliedParams);
        }
        else if (c == 't')
        {
            ok = applyModeT(sender, *ch, adding, channelName, modeParams, paramIdx, appliedModes, appliedParams);
        }
        else if (c == 'k')
        {
            ok = applyModeK(sender, *ch, adding, channelName, modeParams, paramIdx, appliedModes, appliedParams);
        }
        else if (c == 'l')
        {
            ok = applyModeL(sender, *ch, adding, channelName, modeParams, paramIdx, appliedModes, appliedParams);
        }
        else if (c == 'o')
        {
            ok = applyModeO(sender, *ch, adding, channelName, modeParams, paramIdx, appliedModes, appliedParams);
        }
        else
        {
            sender.sendMessage(":ircserv 472 " + sender.getNickname() + " " + c + " :is unknown mode char to me\r\n");
        }

        if (!ok)
            return;
    }

    // Eğer +i/+t/+k/+l için mod değişikliği uygulandıysa kanala bildir
    // (+o zaten broadcast içinde bildirildi, burada tekrar gönderme)
    if (!appliedModes.empty())
    {
        // +o satırlarını zaten broadcast ettik; diğerlerini tek mesajla gönder
        // Basitlik için tüm değişiklikleri tek seferde bildiririz
        std::string prefix = ":" + sender.getNickname() + "!" +
                             sender.getUsername() + "@localhost";
        std::string modeMsg = prefix + " MODE " + channelName + " " +
                              appliedModes + appliedParams + "\r\n";
        ch->broadcast(modeMsg);

        std::cout << "MODE: " << sender.getNickname() << " set " << appliedModes
                  << appliedParams << " on " << channelName << std::endl;
    }
}
