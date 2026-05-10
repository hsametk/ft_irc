#include "../include/Channel.hpp"
#include "../include/Server.hpp"

// Private: Remove Client
void Server::removeClient(int fd) {
  // Poll setinden çıkar.
  for (size_t i = 0; i < _pfds.size(); ++i) {
    if (_pfds[i].fd == fd) {
      _pfds.erase(_pfds.begin() + i);
      break;
    }
  }

  // Tüm kanallardan bu client'ı çıkar.
  // Client quit/disconnect ederken kanaldaki diğer üyelere bildir.
  std::map<int, Client>::iterator clientIt = _clients.find(fd);
  if (clientIt != _clients.end()) {
    Client &client = clientIt->second;
    std::string prefix =
        client.getNickname() + "!" + client.getUsername() + "@localhost";
    std::string quitMsg = ":" + prefix + " QUIT :connection closed\r\n";

    std::map<std::string, Channel>::iterator ch = _channels.begin();
    while (ch != _channels.end()) {
      if (ch->second.hasMember(fd)) {
        // Kanaldaki diğer üyelere QUIT bildir, ardından üyeyi sil.
        ch->second.broadcast(quitMsg, fd);
        ch->second.removeMember(fd);
        // Boş kalan kanalı temizle.
        if (ch->second.getMembers().empty()) {
          std::cout << "Channel " << ch->first << " is now empty, removing."
                    << std::endl;
          _channels.erase(ch++);
          continue;
        }
      }
      ++ch;
    }
  }

  // Soketi kapat ve clients map'inden sil.
  close(fd);
  _clients.erase(fd);
  std::cout << "Client removed: fd=" << fd << std::endl;
}

// --- JOIN ---
void Server::joinChannel(Client &client, const std::string &params) {
  std::string channelName;
  std::string key;
  size_t spacePos = params.find(' ');

  if (spacePos != std::string::npos) {
    channelName = params.substr(0, spacePos);
    key = params.substr(spacePos + 1);
  } else {
    channelName = params;
  }

  // Channel name must start with '#'
  if (channelName.empty() || channelName[0] != '#') {
    sendError(client, ERR_NOSUCHCHANNEL, channelName + " :No such channel");
    return;
  }

  // Check if channel exists; track whether we're creating it
  bool isNewChannel = (_channels.find(channelName) == _channels.end());
  if (isNewChannel) {
    // Create new channel
    _channels[channelName] = Channel(channelName);
  }
  std::map<std::string, Channel>::iterator it = _channels.find(channelName);

  // Check if client is already in channel
  if (it->second.hasMember(client.getFd())) {
    sendError(client, ERR_USERONCHANNEL,
              client.getNickname() + " " + channelName +
                  " :is already on channel");
    return;
  }

  // Check channel key if required
  if (!it->second.getKey().empty() && it->second.getKey() != key) {
    sendError(client, ERR_BADCHANNELKEY,
              channelName + " :Cannot join channel (+k)");
    return;
  }

  // Check invite-only
  if (it->second.isInviteOnly() && !it->second.isInvited(client.getFd())) {
    sendError(client, ERR_INVITEONLYCHAN,
              channelName + " :Cannot join channel (+i)");
    return;
  }

  // Check user limit
  if (it->second.getLimit() > 0 &&
      (int)it->second.getMembers().size() >= it->second.getLimit()) {
    sendError(client, ERR_CHANNELISFULL,
              channelName + " :Cannot join channel (+l)");
    return;
  }

  // Add client to channel — kanalı oluşturan kişi otomatik operator olur.
  it->second.addMember(&client, isNewChannel);
  it->second.removeInvited(client.getFd()); // Katıldıysa davet listesinden çıkar

  // Send JOIN message to client
  std::string prefix =
      client.getNickname() + "!" + client.getUsername() + "@localhost";
  std::string joinMsg = ":" + prefix + " JOIN " + channelName + "\r\n";
  client.sendMessage(joinMsg);

  // Send topic if exists
  if (!it->second.getTopic().empty()) {
    std::string topicMsg = ":ircserv 332 " + client.getNickname() + " " +
                           channelName + " :" + it->second.getTopic() + "\r\n";
    client.sendMessage(topicMsg);
  } else {
    // No topic set → 331 RPL_NOTOPIC
    std::string noTopicMsg = ":ircserv 331 " + client.getNickname() + " " +
                             channelName + " :No topic is set\r\n";
    client.sendMessage(noTopicMsg);
  }

  // Send NAMES list (353 + 366)
  sendNamesList(client, it->second);

  // Notify other clients in channel
  std::string notifyMsg = ":" + prefix + " JOIN " + channelName + "\r\n";
  it->second.broadcast(notifyMsg, client.getFd());
}

// --- PART ---
void Server::partChannel(Client &client, const std::string &params) {
  // params: "#kanal" veya "#kanal :neden"
  std::string channelName;
  std::string reason;

  size_t spacePos = params.find(' ');
  if (spacePos != std::string::npos) {
    channelName = params.substr(0, spacePos);
    reason = params.substr(spacePos + 1);
    // Trailing ':' kaldır
    if (!reason.empty() && reason[0] == ':')
      reason = reason.substr(1);
  } else {
    channelName = params;
    reason = "";
  }

  // Sondaki boşluk / CR temizle
  size_t end = channelName.size();
  while (end > 0 &&
         (channelName[end - 1] == ' ' || channelName[end - 1] == '\r'))
    --end;
  channelName = channelName.substr(0, end);

  // Kanal mevcut mu?
  std::map<std::string, Channel>::iterator it = _channels.find(channelName);
  if (it == _channels.end()) {
    sendError(client, ERR_NOSUCHCHANNEL, channelName + " :No such channel");
    return;
  }

  // Client kanalda mı?
  if (!it->second.hasMember(client.getFd())) {
    // 442 ERR_NOTONCHANNEL
    std::string errMsg = ":ircserv 442 " + client.getNickname() + " " +
                         channelName + " :You're not on that channel\r\n";
    client.sendMessage(errMsg);
    return;
  }

  // Kanaldaki herkese (ayrılan dahil) PART bildir.
  std::string prefix =
      client.getNickname() + "!" + client.getUsername() + "@localhost";
  std::string partMsg = ":" + prefix + " PART " + channelName;
  if (!reason.empty())
    partMsg += " :" + reason;
  partMsg += "\r\n";

  // Ayrılan kişiye de gönder, sonra kanaldan çıkar.
  client.sendMessage(partMsg);
  it->second.broadcast(partMsg, client.getFd());
  it->second.removeMember(client.getFd());

  std::cout << "PART: " << client.getNickname() << " left " << channelName
            << std::endl;

  // Boş kalan kanalı temizle.
  if (it->second.getMembers().empty()) {
    std::cout << "Channel " << channelName << " is now empty, removing."
              << std::endl;
    _channels.erase(it);
  }
}

// --- PRIVMSG ---
void Server::sendServerMessage(Client &client, const std::string &params) {
  // params: "target :message"
  if (params.empty()) {
    sendError(client, 411, ":No recipient given (PRIVMSG)");
    return;
  }

  size_t spacePos = params.find(' ');
  if (spacePos == std::string::npos) {
    sendError(client, 412, ":No text to send");
    return;
  }

  std::string target = params.substr(0, spacePos);
  std::string message = params.substr(spacePos + 1);

  if (message.empty() || (message[0] == ':' && message.size() == 1)) {
    sendError(client, 412, ":No text to send");
    return;
  }

  if (!message.empty() && message[0] == ':')
    message = message.substr(1);

  std::string prefix =
      ":" + client.getNickname() + "!" + client.getUsername() + "@localhost";
  std::string fullMsg = prefix + " PRIVMSG " + target + " :" + message + "\r\n";

  // Kanala mesaj gönderimi
  if (target[0] == '#') {
    std::map<std::string, Channel>::iterator it = _channels.find(target);
    if (it == _channels.end()) {
      sendError(client, 403, target + " :No such channel");
      return;
    }

    // Kullanıcı kanalda mı? (Opsiyonel ama genelde istenir)
    if (!it->second.hasMember(client.getFd())) {
      sendError(client, 404, target + " :Cannot send to channel");
      return;
    }

    // Kanaldaki herkese gönder (gönderen hariç)
    it->second.broadcast(fullMsg, client.getFd());
  }
  // Kullanıcıya mesaj gönderimi
  else {
    bool found = false;
    for (std::map<int, Client>::iterator it = _clients.begin();
         it != _clients.end(); ++it) {
      if (it->second.getNickname() == target) {
        it->second.sendMessage(fullMsg);
        found = true;
        break;
      }
    }

    if (!found)
      sendError(client, 401, target + " :No such nick/channel");
  }
}


// --- NAMES ---
// JOIN sonrası RPL_NAMREPLY (353) ve RPL_ENDOFNAMES (366) gönderir.
// irssi gibi client'lar kanal üye listesini bu mesajlarla oluşturur.
void Server::sendNamesList(Client &client, Channel &channel) {
  std::string names;
  const std::map<int, Client *> &members = channel.getMembers();

  for (std::map<int, Client *>::const_iterator it = members.begin();
       it != members.end(); ++it) {
    if (!names.empty())
      names += " ";
    // Operator'ler @ prefix'i ile gösterilir.
    if (channel.isOperator(it->first))
      names += "@";
    names += it->second->getNickname();
  }

  // 353 RPL_NAMREPLY: "= " public channel anlamına gelir.
  client.sendMessage(":ircserv 353 " + client.getNickname() + " = " +
                     channel.getName() + " :" + names + "\r\n");
  // 366 RPL_ENDOFNAMES
  client.sendMessage(":ircserv 366 " + client.getNickname() + " " +
                     channel.getName() + " :End of /NAMES list\r\n");
}