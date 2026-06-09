#include "test_framework.h"
#include <string.h>

int hora_a_minutos(const char *hora);
int comparar_horas(const char *hora1, const char *hora2);
int hay_solapamiento(const char *inicio1, const char *fin1,
                     const char *inicio2, const char *fin2);
int verificarHora(char *hora);
int validar_horario(const char *hora);

/* ---- hora_a_minutos ---- */
static int test_hora_a_minutos_basico(void) {
    ASSERT_EQ(  0, hora_a_minutos("00:00"));
    ASSERT_EQ( 60, hora_a_minutos("01:00"));
    ASSERT_EQ( 90, hora_a_minutos("01:30"));
    ASSERT_EQ(540, hora_a_minutos("09:00"));
    ASSERT_EQ(780, hora_a_minutos("13:00"));
    ASSERT_EQ(1439, hora_a_minutos("23:59"));
    return 1;
}
static int test_hora_a_minutos_mediodia(void) {
    ASSERT_EQ(720, hora_a_minutos("12:00"));
    ASSERT_EQ(765, hora_a_minutos("12:45"));
    return 1;
}

/* ---- comparar_horas ---- */
static int test_comparar_horas_menor(void) {
    ASSERT_EQ(-1, comparar_horas("08:00", "09:00"));
    ASSERT_EQ(-1, comparar_horas("00:00", "23:59"));
    ASSERT_EQ(-1, comparar_horas("12:00", "12:01"));
    return 1;
}
static int test_comparar_horas_igual(void) {
    ASSERT_EQ(0, comparar_horas("09:00", "09:00"));
    ASSERT_EQ(0, comparar_horas("00:00", "00:00"));
    ASSERT_EQ(0, comparar_horas("23:59", "23:59"));
    return 1;
}
static int test_comparar_horas_mayor(void) {
    ASSERT_EQ(1, comparar_horas("10:00", "09:00"));
    ASSERT_EQ(1, comparar_horas("23:59", "00:00"));
    ASSERT_EQ(1, comparar_horas("12:01", "12:00"));
    return 1;
}

/* ---- hay_solapamiento ---- */
static int test_solapamiento_ninguno(void) {
    ASSERT_EQ(0, hay_solapamiento("09:00", "10:00", "10:00", "11:00"));
    ASSERT_EQ(0, hay_solapamiento("09:00", "10:00", "11:00", "12:00"));
    ASSERT_EQ(0, hay_solapamiento("14:00", "15:00", "10:00", "14:00"));
    return 1;
}
static int test_solapamiento_total(void) {
    ASSERT_EQ(1, hay_solapamiento("09:00", "12:00", "10:00", "11:00"));
    ASSERT_EQ(1, hay_solapamiento("10:00", "11:00", "09:00", "12:00"));
    return 1;
}
static int test_solapamiento_parcial(void) {
    ASSERT_EQ(1, hay_solapamiento("09:00", "11:00", "10:00", "12:00"));
    ASSERT_EQ(1, hay_solapamiento("10:00", "12:00", "09:00", "11:00"));
    return 1;
}
static int test_solapamiento_tocando(void) {
    ASSERT_EQ(0, hay_solapamiento("09:00", "10:00", "10:00", "11:00"));
    ASSERT_EQ(0, hay_solapamiento("10:00", "11:00", "09:00", "10:00"));
    return 1;
}

/* ---- verificarHora ---- */
static int test_verificarHora_valida(void) {
    char h[6]; int r;
    strcpy(h, "09:00"); SILENT_CALL(r, verificarHora(h)); ASSERT_EQ(0, r);
    strcpy(h, "00:00"); SILENT_CALL(r, verificarHora(h)); ASSERT_EQ(0, r);
    strcpy(h, "23:59"); SILENT_CALL(r, verificarHora(h)); ASSERT_EQ(0, r);
    strcpy(h, "12:30"); SILENT_CALL(r, verificarHora(h)); ASSERT_EQ(0, r);
    return 1;
}
static int test_verificarHora_horas_invalidas(void) {
    char h[8]; int r;
    strcpy(h, "24:00"); SILENT_CALL(r, verificarHora(h)); ASSERT_EQ(1, r);
    strcpy(h, "25:00"); SILENT_CALL(r, verificarHora(h)); ASSERT_EQ(1, r);
    return 1;
}
static int test_verificarHora_minutos_invalidos(void) {
    char h[8]; int r;
    strcpy(h, "12:60"); SILENT_CALL(r, verificarHora(h)); ASSERT_EQ(1, r);
    strcpy(h, "12:99"); SILENT_CALL(r, verificarHora(h)); ASSERT_EQ(1, r);
    return 1;
}

/* ---- validar_horario ---- */
static int test_validar_horario_dentro(void) {
    ASSERT_EQ(1, validar_horario("09:00"));
    ASSERT_EQ(1, validar_horario("21:00"));
    ASSERT_EQ(1, validar_horario("14:30"));
    return 1;
}
static int test_validar_horario_fuera(void) {
    ASSERT_EQ(0, validar_horario("08:59"));
    ASSERT_EQ(0, validar_horario("21:01"));
    ASSERT_EQ(0, validar_horario("23:00"));
    return 1;
}

int main(void) {
    RUN_TEST(test_hora_a_minutos_basico,   "Convierte horas comunes correctamente");
    RUN_TEST(test_hora_a_minutos_mediodia, "Convierte el mediodia correctamente");
    RUN_TEST(test_comparar_horas_menor, "Detecta hora anterior");
    RUN_TEST(test_comparar_horas_igual, "Detecta horas iguales");
    RUN_TEST(test_comparar_horas_mayor, "Detecta hora posterior");
    RUN_TEST(test_solapamiento_ninguno,  "Sin solapamiento");
    RUN_TEST(test_solapamiento_total,    "Una franja contiene a la otra");
    RUN_TEST(test_solapamiento_parcial,  "Solapamiento parcial");
    RUN_TEST(test_solapamiento_tocando,  "Franjas que se tocan pero no se solapan");
    RUN_TEST(test_verificarHora_valida,            "Acepta horas validas");
    RUN_TEST(test_verificarHora_horas_invalidas,   "Rechaza hora >= 24");
    RUN_TEST(test_verificarHora_minutos_invalidos, "Rechaza minutos >= 60");
    RUN_TEST(test_validar_horario_dentro, "Acepta horas dentro del horario");
    RUN_TEST(test_validar_horario_fuera,  "Rechaza horas fuera del horario");
    PRINT_RESULTS();
    return _tests_failed ? 1 : 0;
}
