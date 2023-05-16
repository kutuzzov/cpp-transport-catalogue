/*
 * Примерная структура программы:
 *
 * Считать JSON из stdin
 * Построить на его основе JSON базу данных транспортного справочника
 * Выполнить запросы к справочнику, находящиеся в массиве "stat_requests", построив JSON-массив
 * с ответами.
 * Вывести в stdout ответы в виде JSON
 */

#include "json_reader.h"
#include "request_handler.h"

int main() {
#ifdef __APPLE__
    freopen("input.json", "r", stdin);
    //freopen("output.json", "w", stdout);
    //freopen("error-output.txt", "w", stderr);
#endif

    transport::Catalogue catalogue;
    JsonReader requests(std::cin);
    requests.FillCatalogue(catalogue);
    RequestHandler(requests, catalogue);
}