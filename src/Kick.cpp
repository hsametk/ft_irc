#include "../include/Server.hpp"
#include "../include/CmdHelpers.hpp"

// --- KICK ---
// Format: KICK #channel nick [:reason]
//
// Kontrol sırası (subject gereksinimi):
//   1. Parametre eksik mi?       → ERR_NEEDMOREPARAMS (461)  [Auth.cpp'de yapılır]
//   2. Kanal var mı?             → ERR_NOSUCHCHANNEL  (403)
//   3. Sender kanalda mı?        → ERR_NOTONCHANNEL   (442)
//   4. Sender operator mü?       → ERR_CHANOPRIVSNEEDED (482)
//   5. Hedef nick var mı?        → ERR_NOSUCHNICK     (401)
//   6. Hedef kanalda mı?         → ERR_NOTONCHANNEL   (442)
//
// Başarı durumunda:
//   - KICK mesajı tüm kanal üyelerine broadcast edilir (hedef dahil).
//   - Hedef client kanaldan çıkarılır.
//   - Kanal boş kalırsa silinir.
void Server::executeKick(Client &sender, Channel &ch, Client &target,
                         const std::string &channelName,
                         const std::string &targetNick, const std::string &reason)
{
    // --- Başarı: KICK mesajını yayınla ---
    // :nick!user@host KICK #channel targetNick :reason
    std::string prefix = ":" + sender.getNickname() + "!" +
                         sender.getUsername() + "@localhost";
    std::string kickMsg = prefix + " KICK " + channelName + " " +
                          targetNick + " :" + reason + "\r\n";

    // Tüm kanal üyelerine gönder (target dahil, böylece target da bilgili olur).
    ch.broadcast(kickMsg);

    // Hedefi kanaldan çıkar.
    ch.removeMember(target.getFd());

    // Kanal boş kaldıysa sil.
    if (ch.getMembers().empty())
    {
        _channels.erase(channelName);
        std::cout << "Channel " << channelName << " is now empty after KICK, removing." << std::endl;
    }

    std::cout << "KICK: " << sender.getNickname() << " kicked " << targetNick
              << " from " << channelName << " (" << reason << ")" << std::endl;
}

void Server::handleKick(Client &sender, const std::string &channelName,
                        const std::string &targetNick, const std::string &reason)
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

    // 5. Hedef nick var mı?
    Client *target = CmdHelpers_getClientByNick(*this, sender, targetNick);
    if (!target)
        return;

    // 6. Hedef kanalda mı?
    if (!CmdHelpers_requireTargetMember(*this, sender, *ch, *target))
        return;

    executeKick(sender, *ch, *target, channelName, targetNick, reason);
}
