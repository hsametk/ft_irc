#ifndef CMDHELPERS_HPP
#define CMDHELPERS_HPP

#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include <string>
#include <map>

/*
 * CmdHelpers — Shared validation utilities for channel operator commands.
 *
 * Error check order (per IRC spec and subject requirements):
 *   1. Missing parameters      -> ERR_NEEDMOREPARAMS  (461)
 *   2. Channel not found       -> ERR_NOSUCHCHANNEL   (403)
 *   3. Caller not in channel   -> ERR_NOTONCHANNEL    (442)
 *   4. Caller not operator     -> ERR_CHANOPRIVSNEEDED(482)
 *   5. Target nick not found   -> ERR_NOSUCHNICK      (401)
 *   6. Target not in channel   -> ERR_NOTONCHANNEL    (442) for target
 *
 * Return semantics:
 *   - Pointer-returning functions return NULL on failure (error already sent).
 *   - Boolean functions return false on failure (error already sent).
 *   Callers must return immediately after a NULL / false result.
 */

/*
 * Look up a channel by name.
 * Sends ERR_NOSUCHCHANNEL (403) to caller if not found.
 * Returns a pointer to the Channel, or NULL.
 */
Channel *CmdHelpers_getChannelOrError(Server &server, Client &caller,
                                      const std::string &channelName);

/*
 * Look up a connected client by nickname.
 * Sends ERR_NOSUCHNICK (401) to caller if not found.
 * Returns a pointer to the Client, or NULL.
 */
Client *CmdHelpers_getClientByNick(Server &server, Client &caller,
                                   const std::string &nick);

/*
 * Verify that 'caller' is a member of 'channel'.
 * Sends ERR_NOTONCHANNEL (442) if not.
 * Returns true if the caller is a member.
 */
bool CmdHelpers_requireMember(Server &server, Client &caller, Channel &channel);

/*
 * Verify that 'caller' is a channel operator.
 * Sends ERR_CHANOPRIVSNEEDED (482) if not.
 * Returns true if the caller is an operator.
 */
bool CmdHelpers_requireOperator(Server &server, Client &caller, Channel &channel);

/*
 * Verify that 'target' is a member of 'channel'.
 * Sends ERR_NOTONCHANNEL (442) targeted at targetNick if not.
 * Returns true if the target is a member.
 */
bool CmdHelpers_requireTargetMember(Server &server, Client &caller,
                                    Channel &channel, Client &target);

#endif
