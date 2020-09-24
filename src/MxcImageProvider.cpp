#include "MxcImageProvider.h"

#include "Cache.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "Utils.h"

void
MxcImageResponse::run()
{
        if (m_requestedSize.isValid() && !m_encryptionInfo) {
                QString fileName = QString("%1_%2x%3_crop")
                                     .arg(m_id)
                                     .arg(m_requestedSize.width())
                                     .arg(m_requestedSize.height());

                auto data = cache::image(fileName);
                if (!data.isNull()) {
                        m_image = utils::readImage(&data);

                        if (!m_image.isNull()) {
                                m_image = m_image.scaled(
                                  m_requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                                m_image.setText("mxc url", "mxc://" + m_id);

                                if (!m_image.isNull()) {
                                        emit finished();
                                        return;
                                }
                        }
                }

                mtx::http::ThumbOpts opts;
                opts.mxc_url = "mxc://" + m_id.toStdString();
                opts.width   = m_requestedSize.width() > 0 ? m_requestedSize.width() : -1;
                opts.height  = m_requestedSize.height() > 0 ? m_requestedSize.height() : -1;
                opts.method  = "crop";
                http::client()->get_thumbnail(
                  opts, [this, fileName](const std::string &res, mtx::http::RequestErr err) {
                          if (err || res.empty()) {
                                  nhlog::net()->error("Failed to download image {}",
                                                      m_id.toStdString());
                                  m_error = "Failed download";
                                  emit finished();

                                  return;
                          }

                          auto data = QByteArray(res.data(), res.size());
                          cache::saveImage(fileName, data);
                          m_image = utils::readImage(&data);
                          if (!m_image.isNull()) {
                                  m_image = m_image.scaled(
                                    m_requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                          }
                          m_image.setText("mxc url", "mxc://" + m_id);

                          emit finished();
                  });
        } else {
                auto data = cache::image(m_id);

                if (!data.isNull()) {
                        m_image = utils::readImage(&data);
                        m_image.setText("mxc url", "mxc://" + m_id);

                        if (!m_image.isNull()) {
                                emit finished();
                                return;
                        }
                }

                http::client()->download(
                  "mxc://" + m_id.toStdString(),
                  [this](const std::string &res,
                         const std::string &,
                         const std::string &originalFilename,
                         mtx::http::RequestErr err) {
                          if (err) {
                                  nhlog::net()->error("Failed to download image {}",
                                                      m_id.toStdString());
                                  m_error = "Failed download";
                                  emit finished();

                                  return;
                          }

                          auto temp = res;
                          if (m_encryptionInfo)
                                  temp = mtx::crypto::to_string(
                                    mtx::crypto::decrypt_file(temp, m_encryptionInfo.value()));

                          auto data = QByteArray(temp.data(), temp.size());
                          cache::saveImage(m_id, data);
                          m_image = utils::readImage(&data);
                          m_image.setText("original filename",
                                          QString::fromStdString(originalFilename));
                          m_image.setText("mxc url", "mxc://" + m_id);

                          emit finished();
                  });
        }
}
