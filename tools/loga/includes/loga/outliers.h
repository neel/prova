#ifndef PROVA_ALIGN_OUTLIERS_H
#define PROVA_ALIGN_OUTLIERS_H

#include <loga/tokenized_collection.h>
#include <armadillo>

namespace prova::loga{

namespace outliers{

arma::vec lof(const prova::loga::tokenized_collection& subcollection);


}

}

#endif // PROVA_ALIGN_OUTLIERS_H
