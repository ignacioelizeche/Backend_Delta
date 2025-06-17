#include "jwt_helper.h"

QString jwt_helper::generateJWT(int userId, const QString &email, const QString &role) {
    // Header
    QJsonObject header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";

    // Payload
    QJsonObject payload;
    payload["userId"] = userId;
    payload["email"] = email;
    payload["role"] = role;
    payload["iat"] = QDateTime::currentSecsSinceEpoch();
    payload["exp"] = QDateTime::currentSecsSinceEpoch() + (24 * 60 * 60); // 24 hours

    QString headerB64 = QJsonDocument(header).toJson(QJsonDocument::Compact).toBase64(QByteArray::Base64UrlEncoding);
    QString payloadB64 = QJsonDocument(payload).toJson(QJsonDocument::Compact).toBase64(QByteArray::Base64UrlEncoding);

    QString data = headerB64 + "." + payloadB64;
    QString secret = "b3f7c9e128d4a6f5e0c6b8f9d2e3a1c4879d5b2e0f3a4c6e1b9d8f2a3e0c7d1f";
    QByteArray signature = QCryptographicHash::hash((data + secret).toUtf8(), QCryptographicHash::Sha256);
    QString signatureB64 = signature.toBase64(QByteArray::Base64UrlEncoding);

    return headerB64 + "." + payloadB64 + "." + signatureB64;
}

QJsonObject jwt_helper::validateJWT(const QString &token) {
    QStringList parts = token.split(".");
    if (parts.size() != 3) return QJsonObject(); // Invalid

    QString data = parts[0] + "." + parts[1];
    QString secret = "b3f7c9e128d4a6f5e0c6b8f9d2e3a1c4879d5b2e0f3a4c6e1b9d8f2a3e0c7d1f";
    QByteArray expectedSignature = QCryptographicHash::hash((data + secret).toUtf8(), QCryptographicHash::Sha256);
    QString expectedSignatureB64 = expectedSignature.toBase64(QByteArray::Base64UrlEncoding);

    if (parts[2] != expectedSignatureB64) return QJsonObject(); // Invalid

    QByteArray payloadData = QByteArray::fromBase64(parts[1].toUtf8(), QByteArray::Base64UrlEncoding);
    QJsonDocument payloadDoc = QJsonDocument::fromJson(payloadData);
    QJsonObject payload = payloadDoc.object();

    qint64 exp = payload["exp"].toInteger();
    if (exp < QDateTime::currentSecsSinceEpoch()) return QJsonObject(); // Expired

    return payload;
}
