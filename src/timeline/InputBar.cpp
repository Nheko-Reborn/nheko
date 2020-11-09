#include "InputBar.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>

#include <mtx/responses/common.hpp>

#include "Cache.h"
#include "ChatPage.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "TimelineModel.h"
#include "UserSettingsPage.h"
#include "Utils.h"

static constexpr size_t INPUT_HISTORY_SIZE = 10;

void
InputBar::paste(bool fromMouse)
{
        const QMimeData *md = nullptr;

        if (fromMouse) {
                if (QGuiApplication::clipboard()->supportsSelection()) {
                        md = QGuiApplication::clipboard()->mimeData(QClipboard::Selection);
                }
        } else {
                md = QGuiApplication::clipboard()->mimeData(QClipboard::Clipboard);
        }

        if (!md)
                return;

        if (md->hasImage()) {
        } else if (md->hasText()) {
                emit insertText(md->text());
        } else {
                nhlog::ui()->debug("formats: {}", md->formats().join(", ").toStdString());
        }
}

void
InputBar::updateState(int selectionStart_, int selectionEnd_, int cursorPosition_, QString text_)
{
        selectionStart = selectionStart_;
        selectionEnd   = selectionEnd_;
        cursorPosition = cursorPosition_;
        text           = text_;
}

void
InputBar::send()
{
        if (text.trimmed().isEmpty())
                return;

        if (history_.size() == INPUT_HISTORY_SIZE)
                history_.pop_back();
        history_.push_front(text);
        history_index_ = 0;

        if (text.startsWith('/')) {
                int command_end = text.indexOf(' ');
                if (command_end == -1)
                        command_end = text.size();
                auto name = text.mid(1, command_end - 1);
                auto args = text.mid(command_end + 1);
                if (name.isEmpty() || name == "/") {
                        message(args);
                } else {
                        command(name, args);
                }
        } else {
                message(text);
        }

        nhlog::ui()->debug("Send: {}", text.toStdString());
}

void
InputBar::message(QString msg)
{
        mtx::events::msg::Text text = {};
        text.body                   = msg.trimmed().toStdString();

        if (ChatPage::instance()->userSettings()->markdown()) {
                text.formatted_body = utils::markdownToHtml(msg).toStdString();

                // Don't send formatted_body, when we don't need to
                if (text.formatted_body.find("<") == std::string::npos)
                        text.formatted_body = "";
                else
                        text.format = "org.matrix.custom.html";
        }

        if (!room->reply().isEmpty()) {
                auto related = room->relatedInfo(room->reply());

                QString body;
                bool firstLine = true;
                for (const auto &line : related.quoted_body.split("\n")) {
                        if (firstLine) {
                                firstLine = false;
                                body = QString("> <%1> %2\n").arg(related.quoted_user).arg(line);
                        } else {
                                body = QString("%1\n> %2\n").arg(body).arg(line);
                        }
                }

                text.body = QString("%1\n%2").arg(body).arg(msg).toStdString();

                // NOTE(Nico): rich replies always need a formatted_body!
                text.format = "org.matrix.custom.html";
                if (ChatPage::instance()->userSettings()->markdown())
                        text.formatted_body =
                          utils::getFormattedQuoteBody(related, utils::markdownToHtml(msg))
                            .toStdString();
                else
                        text.formatted_body =
                          utils::getFormattedQuoteBody(related, msg.toHtmlEscaped()).toStdString();

                text.relates_to.in_reply_to.event_id = related.related_event;
                room->resetReply();
        }

        room->sendMessageEvent(text, mtx::events::EventType::RoomMessage);
}

void
InputBar::emote(QString msg)
{
        auto html = utils::markdownToHtml(msg);

        mtx::events::msg::Emote emote;
        emote.body = msg.trimmed().toStdString();

        if (html != msg.trimmed().toHtmlEscaped() &&
            ChatPage::instance()->userSettings()->markdown()) {
                emote.formatted_body = html.toStdString();
                emote.format         = "org.matrix.custom.html";
        }

        if (!room->reply().isEmpty()) {
                emote.relates_to.in_reply_to.event_id = room->reply().toStdString();
                room->resetReply();
        }

        room->sendMessageEvent(emote, mtx::events::EventType::RoomMessage);
}

void
InputBar::command(QString command, QString args)
{
        if (command == "me") {
                emote(args);
        } else if (command == "join") {
                ChatPage::instance()->joinRoom(args);
        } else if (command == "invite") {
                ChatPage::instance()->inviteUser(args.section(' ', 0, 0), args.section(' ', 1, -1));
        } else if (command == "kick") {
                ChatPage::instance()->kickUser(args.section(' ', 0, 0), args.section(' ', 1, -1));
        } else if (command == "ban") {
                ChatPage::instance()->banUser(args.section(' ', 0, 0), args.section(' ', 1, -1));
        } else if (command == "unban") {
                ChatPage::instance()->unbanUser(args.section(' ', 0, 0), args.section(' ', 1, -1));
        } else if (command == "roomnick") {
                mtx::events::state::Member member;
                member.display_name = args.toStdString();
                member.avatar_url =
                  cache::avatarUrl(room->roomId(),
                                   QString::fromStdString(http::client()->user_id().to_string()))
                    .toStdString();
                member.membership = mtx::events::state::Membership::Join;

                http::client()->send_state_event(
                  room->roomId().toStdString(),
                  http::client()->user_id().to_string(),
                  member,
                  [](mtx::responses::EventId, mtx::http::RequestErr err) {
                          if (err)
                                  nhlog::net()->error("Failed to set room displayname: {}",
                                                      err->matrix_error.error);
                  });
        } else if (command == "shrug") {
                message("¯\\_(ツ)_/¯" + (args.isEmpty() ? "" : " " + args));
        } else if (command == "fliptable") {
                message("(╯°□°)╯︵ ┻━┻");
        } else if (command == "unfliptable") {
                message(" ┯━┯╭( º _ º╭)");
        } else if (command == "sovietflip") {
                message("ノ┬─┬ノ ︵ ( \\o°o)\\");
        } else if (command == "clear-timeline") {
                room->clearTimeline();
        } else if (command == "rotate-megolm-session") {
                cache::dropOutboundMegolmSession(room->roomId().toStdString());
        }
}
