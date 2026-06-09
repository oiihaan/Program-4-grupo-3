#include "test_framework.h"
#include "../include/funciones.h"

/* ---- dni_es_valido ---- */
static int test_dni_valido_conocidos(void) {
    int r;
    SILENT_CALL(r, dni_es_valido("00000000T")); ASSERT_EQ(1, r);
    SILENT_CALL(r, dni_es_valido("00000001R")); ASSERT_EQ(1, r);
    SILENT_CALL(r, dni_es_valido("12345678Z")); ASSERT_EQ(1, r);
    SILENT_CALL(r, dni_es_valido("11111111H")); ASSERT_EQ(1, r);
    SILENT_CALL(r, dni_es_valido("87654321X")); ASSERT_EQ(1, r);
    SILENT_CALL(r, dni_es_valido("99999999R")); ASSERT_EQ(1, r);
    return 1;
}
static int test_dni_letra_minuscula(void) {
    int r;
    SILENT_CALL(r, dni_es_valido("00000000t")); ASSERT_EQ(1, r);
    SILENT_CALL(r, dni_es_valido("12345678z")); ASSERT_EQ(1, r);
    return 1;
}
static int test_dni_letra_incorrecta(void) {
    int r;
    SILENT_CALL(r, dni_es_valido("12345678X")); ASSERT_EQ(0, r);
    SILENT_CALL(r, dni_es_valido("00000000A")); ASSERT_EQ(0, r);
    return 1;
}
static int test_dni_longitud_incorrecta(void) {
    int r;
    SILENT_CALL(r, dni_es_valido("1234567Z"));  ASSERT_EQ(0, r);
    SILENT_CALL(r, dni_es_valido("123456789Z")); ASSERT_EQ(0, r);
    SILENT_CALL(r, dni_es_valido(""));           ASSERT_EQ(0, r);
    return 1;
}
static int test_dni_numero_invalido(void) {
    int r;
    SILENT_CALL(r, dni_es_valido("1234A678Z")); ASSERT_EQ(0, r);
    SILENT_CALL(r, dni_es_valido("ABCDEFGHT")); ASSERT_EQ(0, r);
    return 1;
}
static int test_dni_noveno_no_letra(void) {
    int r;
    SILENT_CALL(r, dni_es_valido("123456781")); ASSERT_EQ(0, r);
    return 1;
}

/* ---- fecha_es_valida ---- */
static int test_fecha_valida_normal(void) {
    int r;
    SILENT_CALL(r, fecha_es_valida("2026-06-09")); ASSERT_EQ(1, r);
    SILENT_CALL(r, fecha_es_valida("2000-01-01")); ASSERT_EQ(1, r);
    SILENT_CALL(r, fecha_es_valida("1999-12-31")); ASSERT_EQ(1, r);
    return 1;
}
static int test_fecha_bisiesto(void) {
    int r;
    SILENT_CALL(r, fecha_es_valida("2024-02-29")); ASSERT_EQ(1, r);
    SILENT_CALL(r, fecha_es_valida("2023-02-29")); ASSERT_EQ(0, r);
    SILENT_CALL(r, fecha_es_valida("2000-02-29")); ASSERT_EQ(1, r);
    SILENT_CALL(r, fecha_es_valida("1900-02-29")); ASSERT_EQ(0, r);
    return 1;
}
static int test_fecha_mes_fuera_de_rango(void) {
    int r;
    SILENT_CALL(r, fecha_es_valida("2026-00-01")); ASSERT_EQ(0, r);
    SILENT_CALL(r, fecha_es_valida("2026-13-01")); ASSERT_EQ(0, r);
    return 1;
}
static int test_fecha_dia_fuera_de_rango(void) {
    int r;
    SILENT_CALL(r, fecha_es_valida("2026-01-00")); ASSERT_EQ(0, r);
    SILENT_CALL(r, fecha_es_valida("2026-01-32")); ASSERT_EQ(0, r);
    SILENT_CALL(r, fecha_es_valida("2026-04-31")); ASSERT_EQ(0, r);
    SILENT_CALL(r, fecha_es_valida("2026-06-31")); ASSERT_EQ(0, r);
    return 1;
}
static int test_fecha_formato_incorrecto(void) {
    int r;
    SILENT_CALL(r, fecha_es_valida("26-06-09"));  ASSERT_EQ(0, r);
    SILENT_CALL(r, fecha_es_valida("2026/06/09")); ASSERT_EQ(0, r);
    SILENT_CALL(r, fecha_es_valida("20260609"));   ASSERT_EQ(0, r);
    SILENT_CALL(r, fecha_es_valida(""));           ASSERT_EQ(0, r);
    SILENT_CALL(r, fecha_es_valida("2026-6-9"));   ASSERT_EQ(0, r);
    return 1;
}

/* ---- comprobar_contrasena ---- */
static int test_contrasena_valida(void) {
    int r;
    SILENT_CALL(r, comprobar_contrasena("abc123"));     ASSERT_EQ(1, r);
    SILENT_CALL(r, comprobar_contrasena("x"));          ASSERT_EQ(1, r);
    SILENT_CALL(r, comprobar_contrasena("MiClaveS3g")); ASSERT_EQ(1, r);
    return 1;
}
static int test_contrasena_vacia(void) {
    int r;
    SILENT_CALL(r, comprobar_contrasena(""));   ASSERT_EQ(0, r);
    SILENT_CALL(r, comprobar_contrasena(NULL)); ASSERT_EQ(0, r);
    return 1;
}

int main(void) {
    RUN_TEST(test_dni_valido_conocidos,    "Acepta DNIs con letra correcta");
    RUN_TEST(test_dni_letra_minuscula,     "Acepta letra en minuscula");
    RUN_TEST(test_dni_letra_incorrecta,    "Rechaza letra incorrecta");
    RUN_TEST(test_dni_longitud_incorrecta, "Rechaza longitud incorrecta");
    RUN_TEST(test_dni_numero_invalido,     "Rechaza letras en la parte numerica");
    RUN_TEST(test_dni_noveno_no_letra,     "Rechaza digito en la posicion de letra");
    RUN_TEST(test_fecha_valida_normal,        "Acepta fechas correctas");
    RUN_TEST(test_fecha_bisiesto,             "Maneja anos bisiestos correctamente");
    RUN_TEST(test_fecha_mes_fuera_de_rango,   "Rechaza mes 0 y mes 13");
    RUN_TEST(test_fecha_dia_fuera_de_rango,   "Rechaza dias inexistentes en el mes");
    RUN_TEST(test_fecha_formato_incorrecto,   "Rechaza formatos distintos a YYYY-MM-DD");
    RUN_TEST(test_contrasena_valida, "Acepta contrasenas no vacias");
    RUN_TEST(test_contrasena_vacia,  "Rechaza contrasena vacia o NULL");
    PRINT_RESULTS();
    return _tests_failed ? 1 : 0;
}
