/****************************************************************************
**
** Copyright (C) 2014-2017 Dinu SV.
** (contact: mail@dinusv.com)
** This file is part of Live CV Application.
**
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
****************************************************************************/

#include "qeditorsettings.h"
#include "qprojectdocument.h"
#include "qprojectfile.h"
#include "qeditorsettingscategory.h"
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>

namespace lcv{

QEditorSettings::QEditorSettings(const QString &path, QObject *parent)
    : QObject(parent)
    , m_fontSize(12)
    , m_path(path)
{
}

QEditorSettings::~QEditorSettings(){
}

void QEditorSettings::fromJson(const QJsonObject &root){
    for( QJsonObject::ConstIterator it = root.begin(); it != root.end(); ++it ){
        if ( it.key() == "font" ){
            QJsonObject fontObj = it.value().toObject();
            if ( fontObj.contains("size") ){
                m_fontSize = root["size"].toInt();
                emit fontSizeChanged(m_fontSize);
            }
        } else {
            QEditorSettingsCategory* category = settingsFor(it.key());
            if ( category )
                category->fromJson(it.value());
            else
                qCritical("Failed to find settings for: %s", qPrintable(it.key()));
        }
    }
}

QJsonObject QEditorSettings::toJson() const{
    QJsonObject root;

    QJsonObject font;
    font["size"] = m_fontSize;

    root["font"] = font;

    for ( QHash<QString, QEditorSettingsCategory*>::ConstIterator it = m_settings.begin();
          it != m_settings.end();
          ++it
    ){
        root[it.key()] = it.value()->toJson();
    }

    return root;
}

void QEditorSettings::syncWithFile(){
    QFile file(m_path);
//    if ( !file.exists() ){
        m_content = QJsonDocument(toJson()).toJson(QJsonDocument::Indented);
        if ( file.open(QIODevice::WriteOnly) ){
            file.write(m_content);
            file.close();
        } else {
            qCritical("Failed to open settings file for writting: %s", qPrintable(m_path));
        }
//    } else if ( file.open(QIODevice::ReadOnly) ){
//        init(file.readAll());
//        file.close();
//    }
}

void QEditorSettings::init(const QByteArray &data){
    m_content = data;
    QJsonParseError error;
    QJsonDocument jsondoc = QJsonDocument::fromJson(data, &error);
    if ( error.error != QJsonParseError::NoError ){
        emit initError(error.errorString());
        qCritical("Failed to parse settings file: %s", qPrintable(error.errorString()));
    }
    fromJson(jsondoc.object());
}

void QEditorSettings::documentOpened(lcv::QProjectDocument *document){
    connect(document, SIGNAL(isDirtyChanged()), this, SLOT(documentIsDirtyChanged()));
}

void QEditorSettings::documentIsDirtyChanged(){
    QProjectDocument* document = qobject_cast<QProjectDocument*>(sender());
    if ( document ){
        if ( !document->isDirty() && document->file()->path() == m_path ){
            init(document->content().toUtf8());
        }
    }
}

}// namespace
