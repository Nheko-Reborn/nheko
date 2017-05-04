/*
 * nheko Copyright (C) 2017  Konstantinos Sideris <siderisk@auth.gr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef IMAGE_OVERLAY_DIALOG_H
#define IMAGE_OVERLAY_DIALOG_H

#include <QDialog>
#include <QMouseEvent>
#include <QPixmap>

class ImageOverlayDialog : public QDialog
{
	Q_OBJECT
public:
	ImageOverlayDialog(QPixmap image, QWidget *parent = nullptr);

	void reject() override;

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void paintEvent(QPaintEvent *event) override;

signals:
	void closing();

private slots:
	void closeDialog();

private:
	void scaleImage(int width, int height);

	QPixmap originalImage_;
	QPixmap image_;

	QRect content_;
	QRect close_button_;
};

#endif  // IMAGE_OVERLAY_DIALOG_H
