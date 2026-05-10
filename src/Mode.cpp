#include "../include/Server.hpp"
#include "../include/CmdHelpers.hpp"
#include <cstdlib>   // atoi
#include <cctype>    // isdigit

// ---------------------------------------------------------------------------
// Yardımcı: string'in geçerli pozitif sayı olup olmadığını kontrol eder
// ---------------------------------------------------------------------------
static bool isPositiveNumber(const std::string &s)
{
    if (s.empty())
        return false;
    for (size_t i = 0; i < s.size(); ++i)
        if (!std::isdigit(s[i]))
            return false;
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
    Channel *ch = CmdHelpers::getChannelOrError(*this, sender, channelName);
    if (!ch)
        return;

    // 3. Sender kanalda mı?
    if (!CmdHelpers::requireMember(*this, sender, *ch))
        return;

    // 4. Sender operator mü?
    if (!CmdHelpers::requireOperator(*this, sender, *ch))
        return;

    // Uygulanan mod değişikliklerini RPL_CHANNELMODEIS (324) için biriktir
    std::string appliedModes;
    std::string appliedParams;
    bool        adding = true;  // '+' → true, '-' → false
    size_t      paramIdx = 0;   // modeParams içindeki sıradaki parametre

    for (size_t i = 0; i < modeStr.size(); ++i)
    {
        char c = modeStr[i];

        if (c == '+') { adding = true;  continue; }
        if (c == '-') { adding = false; continue; }

        switch (c)
        {
            // ----------------------------------------------------------
            // +i / -i  — Invite-only
            // ----------------------------------------------------------
            case 'i':
                ch->setInviteOnly(adding);
                appliedModes += (adding ? "+" : "-");
                appliedModes += 'i';
                break;

            // ----------------------------------------------------------
            // +t / -t  — Topic sadece operatörler değiştirebilir
            // ----------------------------------------------------------
            case 't':
                ch->setTopicOpOnly(adding);
                appliedModes += (adding ? "+" : "-");
                appliedModes += 't';
                break;

            // ----------------------------------------------------------
            // +k / -k  — Kanal şifresi
            // ----------------------------------------------------------
            case 'k':
                if (adding)
                {
                    // +k parametre gerektirir
                    if (paramIdx >= modeParams.size() || modeParams[paramIdx].empty())
                    {
                        sendError(sender, ERR_NEEDMOREPARAMS,
                                  channelName + " +k :Not enough parameters");
                        return;
                    }
                    ch->setKey(modeParams[paramIdx]);
                    appliedModes  += "+k";
                    appliedParams += " " + modeParams[paramIdx];
                    ++paramIdx;
                }
                else
                {
                    // -k parametre almaz; şifreyi temizle
                    ch->setKey("");
                    appliedModes += "-k";
                }
                break;

            // ----------------------------------------------------------
            // +l / -l  — Üye limiti
            // ----------------------------------------------------------
            case 'l':
                if (adding)
                {
                    // +l parametre gerektirir ve pozitif sayı olmalı
                    if (paramIdx >= modeParams.size() ||
                        !isPositiveNumber(modeParams[paramIdx]))
                    {
                        sendError(sender, ERR_NEEDMOREPARAMS,
                                  channelName + " +l :Invalid or missing limit");
                        return;
                    }
                    int limit = std::atoi(modeParams[paramIdx].c_str());
                    ch->setLimit(limit);
                    appliedModes  += "+l";
                    appliedParams += " " + modeParams[paramIdx];
                    ++paramIdx;
                }
                else
                {
                    // -l → limiti kaldır
                    ch->setLimit(0);
                    appliedModes += "-l";
                }
                break;

            // ----------------------------------------------------------
            // +o / -o  — Operatör yetkisi ver / al
            // ----------------------------------------------------------
            case 'o':
            {
                if (paramIdx >= modeParams.size() || modeParams[paramIdx].empty())
                {
                    sendError(sender, ERR_NEEDMOREPARAMS,
                              channelName + " +o/-o :Not enough parameters");
                    return;
                }
                const std::string &targetNick = modeParams[paramIdx];
                ++paramIdx;

                // 7. Hedef nick var mı?
                Client *target = CmdHelpers::getClientByNick(*this, sender, targetNick);
                if (!target)
                    return;

                // 8. Hedef kanalda mı?
                if (!CmdHelpers::requireTargetMember(*this, sender, *ch, *target))
                    return;

                if (adding)
                {
                    ch->addOperator(target->getFd());
                    appliedModes  += "+o";
                    appliedParams += " " + targetNick;
                    // Kanaldaki herkese MODE bildir (irssi @ prefix günceller)
                    std::string modeMsg = ":" + sender.getNickname() + "!" +
                                         sender.getUsername() + "@localhost" +
                                         " MODE " + channelName + " +o " +
                                         targetNick + "\r\n";
                    ch->broadcast(modeMsg);
                }
                else
                {
                    ch->removeOperator(target->getFd());
                    appliedModes  += "-o";
                    appliedParams += " " + targetNick;
                    std::string modeMsg = ":" + sender.getNickname() + "!" +
                                         sender.getUsername() + "@localhost" +
                                         " MODE " + channelName + " -o " +
                                         targetNick + "\r\n";
                    ch->broadcast(modeMsg);
                }
                break;
            }

            default:
                // Bilinmeyen mod karakteri → 472 ERR_UNKNOWNMODE
                sender.sendMessage(":ircserv 472 " + sender.getNickname() +
                                   " " + c + " :is unknown mode char to me\r\n");
                break;
        }
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
