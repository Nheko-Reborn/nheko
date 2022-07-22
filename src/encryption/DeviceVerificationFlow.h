// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include <mtx/responses/crypto.hpp>

#include "CacheCryptoStructs.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "Olm.h"
#include "timeline/TimelineModel.h"

class QTimer;

using sas_ptr = std::unique_ptr<mtx::crypto::SAS>;

// clang-format off
/*
 * Stolen from fluffy chat :D
 *
 *      State         |   +-------------+                    +-----------+                                  |
 *                    |   | AliceDevice |                    | BobDevice |                                  |
 *                    |   | (sender)    |                    |           |                                  |
 *                    |   +-------------+                    +-----------+                                  |
 * promptStartVerify  |         |                                 |                                         |
 *                    |      o  | (m.key.verification.request)    |                                         |
 *                    |      p  |-------------------------------->| (ASK FOR VERIFICATION REQUEST)          |
 * waitForOtherAccept |      t  |                                 |                                         | promptStartVerify
 * &&                 |      i  |      (m.key.verification.ready) |                                         |
 * no commitment      |      o  |<--------------------------------|                                         |
 * &&                 |      n  |                                 |                                         |
 * no canonical_json  |      a  |      (m.key.verification.start) |                                         | waitingForKeys
 *                    |      l  |<--------------------------------| Not sending to prevent the glare resolve| && no commitment
 *                    |         |                                 |                               (1)       | && no canonical_json
 *                    |         | m.key.verification.start        |                                         |
 * waitForOtherAccept |         |-------------------------------->| (IF NOT ALREADY ASKED,                  |
 * &&                 |         |                                 |  ASK FOR VERIFICATION REQUEST)          | promptStartVerify, if not accepted
 * canonical_json     |         |       m.key.verification.accept |                                         |
 *                    |         |<--------------------------------|                                         |
 * waitForOtherAccept |         |                                 |                                         | waitingForKeys
 * &&                 |         | m.key.verification.key          |                                         | && canonical_json
 * commitment         |         |-------------------------------->|                                         | && commitment
 *                    |         |                                 |                                         |
 *                    |         |          m.key.verification.key |                                         |
 *                    |         |<--------------------------------|                                         |
 * compareEmoji/Number|         |                                 |                                         | compareEmoji/Number
 *                    |         |     COMPARE EMOJI / NUMBERS     |                                         |
 *                    |         |                                 |                                         |
 * waitingForMac      |         |     m.key.verification.mac      |                                         | waitingForMac
 *                    | success |<------------------------------->|  success                                |
 *                    |         |                                 |                                         |
 * success/fail       |         |         m.key.verification.done |                                         | success/fail
 *                    |         |<------------------------------->|                                         |
 *
 *  (1) Sometimes the other side does send this start. In this case we run the glare algorithm and send an accept only if
 *      We are the bigger mxid and deviceid (since we discard our start message). <- GLARE RESOLUTION
 */
// clang-format on
class DeviceVerificationFlow : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(Error error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString userId READ getUserId CONSTANT)
    Q_PROPERTY(QString deviceId READ getDeviceId CONSTANT)
    Q_PROPERTY(bool sender READ getSender CONSTANT)
    Q_PROPERTY(std::vector<int> sasList READ getSasList CONSTANT)
    Q_PROPERTY(bool isDeviceVerification READ isDeviceVerification CONSTANT)
    Q_PROPERTY(bool isSelfVerification READ isSelfVerification CONSTANT)
    Q_PROPERTY(bool isMultiDeviceVerification READ isMultiDeviceVerification CONSTANT)

public:
    enum State
    {
        PromptStartVerification,
        WaitingForOtherToAccept,
        WaitingForKeys,
        CompareEmoji,
        CompareNumber,
        WaitingForMac,
        Success,
        Failed,
    };
    Q_ENUM(State)

    enum Type
    {
        ToDevice,
        RoomMsg
    };

    enum Error
    {
        UnknownMethod,
        MismatchedCommitment,
        MismatchedSAS,
        KeyMismatch,
        Timeout,
        User,
        OutOfOrder,
    };
    Q_ENUM(Error)

    static QSharedPointer<DeviceVerificationFlow>
    NewInRoomVerification(QObject *parent_,
                          TimelineModel *timelineModel_,
                          const mtx::events::msg::KeyVerificationRequest &msg,
                          QString other_user_,
                          QString event_id_);
    static QSharedPointer<DeviceVerificationFlow>
    NewToDeviceVerification(QObject *parent_,
                            const mtx::events::msg::KeyVerificationRequest &msg,
                            QString other_user_,
                            QString txn_id_);
    static QSharedPointer<DeviceVerificationFlow>
    NewToDeviceVerification(QObject *parent_,
                            const mtx::events::msg::KeyVerificationStart &msg,
                            QString other_user_,
                            QString txn_id_);
    static QSharedPointer<DeviceVerificationFlow>
    InitiateUserVerification(QObject *parent_, TimelineModel *timelineModel_, QString userid);
    static QSharedPointer<DeviceVerificationFlow>
    InitiateDeviceVerification(QObject *parent, QString userid, std::vector<QString> devices);

    // getters
    QString state();
    Error error() { return error_; }
    QString getUserId();
    QString getDeviceId();
    bool getSender();
    std::vector<int> getSasList();
    QString transactionId() { return QString::fromStdString(this->transaction_id); }
    // setters
    void setDeviceId(QString deviceID);
    void setEventId(std::string event_id);
    bool isDeviceVerification() const
    {
        return this->type == DeviceVerificationFlow::Type::ToDevice;
    }
    bool isSelfVerification() const;
    bool isMultiDeviceVerification() const { return deviceIds.size() > 1; }

    void callback_fn(const UserKeyCache &res, mtx::http::RequestErr err, std::string user_id);

public slots:
    //! unverifies a device
    void unverify();
    //! Continues the flow
    void next();
    //! Cancel the flow
    void cancel() { cancelVerification(User); }

signals:
    void refreshProfile();
    void stateChanged();
    void errorChanged();

private:
    DeviceVerificationFlow(QObject *,
                           DeviceVerificationFlow::Type flow_type,
                           TimelineModel *model,
                           QString userID,
                           std::vector<QString> deviceIds_);
    void setState(State state)
    {
        if (state != state_) {
            state_ = state;
            emit stateChanged();
        }
    }

    void handleStartMessage(const mtx::events::msg::KeyVerificationStart &msg, std::string);
    //! sends a verification request
    void sendVerificationRequest();
    //! accepts a verification request
    void sendVerificationReady();
    //! completes the verification flow();
    void sendVerificationDone();
    //! accepts a verification
    void acceptVerificationRequest();
    //! starts the verification flow
    void startVerificationRequest();
    //! cancels a verification flow
    void cancelVerification(DeviceVerificationFlow::Error error_code);
    //! sends the verification key
    void sendVerificationKey();
    //! sends the mac of the keys
    void sendVerificationMac();
    //! Completes the verification flow
    void acceptDevice();

    std::string transaction_id;

    bool sender;
    Type type;
    mtx::identifiers::User toClient;
    QString deviceId;
    std::vector<QString> deviceIds;

    // public part of our master key, when trusted or empty
    std::string our_trusted_master_key;

    mtx::events::msg::SASMethods method = mtx::events::msg::SASMethods::Emoji;
    QTimer *timeout                     = nullptr;
    sas_ptr sas;
    std::string mac_method;
    std::string commitment;
    std::string canonical_json;

    std::vector<int> sasList;
    UserKeyCache their_keys;
    TimelineModel *model_;
    mtx::common::Relation relation;

    State state_ = PromptStartVerification;
    Error error_ = UnknownMethod;

    bool isMacVerified = false;

    bool keySent = false, macSent = false, acceptSent = false, startSent = false;

    template<typename T>
    void send(T msg)
    {
        if (this->type == DeviceVerificationFlow::Type::ToDevice) {
            mtx::requests::ToDeviceMessages<T> body;
            msg.transaction_id = this->transaction_id;
            for (const auto &d : deviceIds)
                body[this->toClient][d.toStdString()] = msg;

            http::client()->send_to_device<T>(
              http::client()->generate_txn_id(), body, [](mtx::http::RequestErr err) {
                  if (err)
                      nhlog::net()->warn("failed to send verification to_device message: {} {}",
                                         err->matrix_error.error,
                                         static_cast<int>(err->status_code));
              });
        } else if (this->type == DeviceVerificationFlow::Type::RoomMsg && model_) {
            if constexpr (!std::is_same_v<T, mtx::events::msg::KeyVerificationRequest>) {
                msg.relations.relations.push_back(this->relation);
                // Set synthesized to surpress the nheko relation extensions
                msg.relations.synthesized = true;
            }
            (model_)->sendMessageEvent(msg, mtx::events::to_device_content_to_type<T>);
        }

        nhlog::net()->debug("Sent verification step: {} in state: {}",
                            mtx::events::to_string(mtx::events::to_device_content_to_type<T>),
                            state().toStdString());
    }
};
