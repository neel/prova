#include <iostream>
#include <cstdint>
#include <map>
#include <memory>
#include <functional>
#include <filesystem>
#include <fstream>
#include "prova/process.h"
#include "prova/artifact.h"
#include "prova/session.h"
#include "prova/action.h"
#include "prova/store.h"
#include "prova/execution_unit.h"

int main(){
    prova::store store;
    store.fetch();
    // store.uml(std::cout);
    store.extract_all();

    return 0;
}
