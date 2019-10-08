#include "MxcImageProvider.h"

#include "Cache.h"

void
MxcImageResponse::run()
{
        if (m_requestedSize.isValid()) {
                QString fileName = QString("%1_%2x%3")
                                     .arg(m_id)
                                     .arg(m_requestedSize.width())
                                     .arg(m_requestedSize.height());

                auto data = cache::client()->image(fileName);
                if (!data.isNull() && m_image.loadFromData(data)) {
                        m_image = m_image.scaled(m_requestedSize, Qt::KeepAspectRatio);
                        m_image.setText("mxc url", "mxc://" + m_id);
                        emit finished();
                        return;
                }

                mtx::http::ThumbOpts opts;
                opts.mxc_url = "mxc://" + m_id.toStdString();
                opts.width   = m_requestedSize.width() > 0 ? m_requestedSize.width() : -1;
                opts.height  = m_requestedSize.height() > 0 ? m_requestedSize.height() : -1;
                opts.method  = "scale";
                http::client()->get_thumbnail(
                  opts, [this, fileName](const std::string &res, mtx::http::RequestErr err) {
                          if (err) {
                                  nhlog::net()->error("Failed to download image {}",
                                                      m_id.toStdString());
                                  m_error = "Failed download";
                                  emit finished();

                                  return;
                          }

                          auto data = QByteArray(res.data(), res.size());
                          cache::client()->saveImage(fileName, data);
                          m_image.loadFromData(data);
                          m_image = m_image.scaled(
                            m_requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                          m_image.setText("mxc url", "mxc://" + m_id);

                          emit finished();
                  });
        } else {
                auto data = cache::client()->image(m_id);
                if (!data.isNull() && m_image.loadFromData(data)) {
                        m_image.setText("mxc url", "mxc://" + m_id);
                        emit finished();
                        return;
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

                          auto data = QByteArray(res.data(), res.size());
                          m_image.loadFromData(data);
                          m_image.setText("original filename",
                                          QString::fromStdString(originalFilename));
                          m_image.setText("mxc url", "mxc://" + m_id);
                          cache::client()->saveImage(m_id, data);

                          emit finished();
                  });
        }
}
