#include "test_framework.h"
#include <string.h>

void invertir_recursivo(char *cad, int inicio, int fin);
void auth_generar_hash(const char *password, const char *fecha, char *out_hash);

/* ---- invertir_recursivo ---- */
static int test_invertir_cadena_normal(void) {
    char s[32];
    strcpy(s, "pablo");
    invertir_recursivo(s, 0, (int)strlen(s) - 1);
    ASSERT_STR_EQ("olbap", s);
    return 1;
}
static int test_invertir_cadena_par(void) {
    char s[32];
    strcpy(s, "abcd");
    invertir_recursivo(s, 0, (int)strlen(s) - 1);
    ASSERT_STR_EQ("dcba", s);
    return 1;
}
static int test_invertir_un_caracter(void) {
    char s[4] = "x";
    invertir_recursivo(s, 0, 0);
    ASSERT_STR_EQ("x", s);
    return 1;
}
static int test_invertir_palindromo(void) {
    char s[16];
    strcpy(s, "ana");
    invertir_recursivo(s, 0, (int)strlen(s) - 1);
    ASSERT_STR_EQ("ana", s);
    return 1;
}
static int test_invertir_dos_veces_restaura(void) {
    char s[32];
    const char *original = "ayuntamiento";
    strcpy(s, original);
    int n = (int)strlen(s) - 1;
    invertir_recursivo(s, 0, n);
    invertir_recursivo(s, 0, n);
    ASSERT_STR_EQ(original, s);
    return 1;
}

/* ---- auth_generar_hash ---- */
static int test_hash_longitud(void) {
    char hash[65];
    auth_generar_hash("mipassword", "2026-01-01 00:00:00", hash);
    ASSERT_EQ(64, (int)strlen(hash));
    return 1;
}
static int test_hash_solo_hex(void) {
    char hash[65];
    auth_generar_hash("test", "2026-06-09 12:00:00", hash);
    for (int i = 0; i < 64; i++) {
        char c = hash[i];
        ASSERT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
    }
    return 1;
}
static int test_hash_deterministico(void) {
    char h1[65], h2[65];
    auth_generar_hash("ClaveSegura99", "2025-05-01 08:30:00", h1);
    auth_generar_hash("ClaveSegura99", "2025-05-01 08:30:00", h2);
    ASSERT_STR_EQ(h1, h2);
    return 1;
}
static int test_hash_diferente_por_password(void) {
    char h1[65], h2[65];
    auth_generar_hash("pass1", "2025-05-01 08:30:00", h1);
    auth_generar_hash("pass2", "2025-05-01 08:30:00", h2);
    ASSERT_TRUE(strcmp(h1, h2) != 0);
    return 1;
}
static int test_hash_diferente_por_sal(void) {
    char h1[65], h2[65];
    auth_generar_hash("mismaPassword", "2025-01-01 00:00:00", h1);
    auth_generar_hash("mismaPassword", "2025-01-02 00:00:00", h2);
    ASSERT_TRUE(strcmp(h1, h2) != 0);
    return 1;
}
static int test_hash_valor_conocido(void) {
    char hash[65], hash2[65];
    auth_generar_hash("abc", "salt", hash);
    auth_generar_hash("abc", "salt", hash2);
    ASSERT_STR_EQ(hash, hash2);
    ASSERT_EQ(64, (int)strlen(hash));
    return 1;
}

int main(void) {
    RUN_TEST(test_invertir_cadena_normal,      "Invierte una cadena de longitud impar");
    RUN_TEST(test_invertir_cadena_par,         "Invierte una cadena de longitud par");
    RUN_TEST(test_invertir_un_caracter,        "Una sola letra no cambia");
    RUN_TEST(test_invertir_palindromo,         "Un palindromo invertido es igual");
    RUN_TEST(test_invertir_dos_veces_restaura, "Invertir dos veces restaura el original");
    RUN_TEST(test_hash_longitud,               "El hash tiene exactamente 64 caracteres");
    RUN_TEST(test_hash_solo_hex,               "El hash solo contiene caracteres hexadecimales");
    RUN_TEST(test_hash_deterministico,         "El mismo input siempre produce el mismo hash");
    RUN_TEST(test_hash_diferente_por_password, "Passwords distintas generan hashes distintos");
    RUN_TEST(test_hash_diferente_por_sal,      "La misma password con distinta sal genera hash distinto");
    RUN_TEST(test_hash_valor_conocido,         "Consistencia interna del algoritmo");
    PRINT_RESULTS();
    return _tests_failed ? 1 : 0;
}
