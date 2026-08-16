#pragma once

#include <string>
#include <string_view>
#include <chrono>
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/defaults.h>
#include <sodium.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace utils::crypto 
{

/// Хэширование сырого пароля через Argon2id
std::string HashPassword(std::string_view password);

/// Безопасная проверка пароля против хэша из PostgreSQL
bool VerifyPassword(std::string_view password, std::string_view password_hash);

/// Генерация Access Token (JWT) (по умолчанию на 30 дней для мобильных клиентов)
std::string GenerateAccessToken(
    uint64_t user_id, 
    std::string_view jwt_secret,
    std::chrono::seconds expires_in = std::chrono::hours(24 * 30)
);

/// Валидация Access Token (JWT) и извлечение user_id
std::optional<uint64_t> VerifyAccessToken(std::string_view token, std::string_view jwt_secret);

/// Генерация прозрачного Opaque Refresh-токена (Простая уникальная строка)
inline std::string GenerateRefreshToken()
{
    boost::uuids::random_generator gen;
    return boost::uuids::to_string(gen());
}

/// Вычисление SHA-256 от Refresh-токена для безопасного хранения в БД
std::string HashSha256(std::string_view data);

} // namespace utils::crypto