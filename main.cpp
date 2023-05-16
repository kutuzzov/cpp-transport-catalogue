/*
 * ��������� ��������� ���������:
 *
 * ������� JSON �� stdin
 * ��������� �� ��� ������ JSON ���� ������ ������������� �����������
 * ��������� ������� � �����������, ����������� � ������� "stat_requests", �������� JSON-������
 * � ��������.
 * ������� � stdout ������ � ���� JSON
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