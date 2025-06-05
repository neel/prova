#include "cveproxymodel.h"
#include "cvelistmodel.h"
#include "keywordlistmodel.h"

CVEProxyModel::CVEProxyModel(QObject *parent): QSortFilterProxyModel{parent}{

}

bool CVEProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const{
    QModelIndex idIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    QModelIndex keywordIndex = sourceModel()->index(sourceRow, 0, sourceParent);

    QString id = sourceModel()->data(idIndex, CVEListModel::IdRole).toString();
    QStringList keywords = sourceModel()->data(keywordIndex, CVEListModel::KeywordRole).toStringList();

    // { permisive if all false return false otherwise proceed
    // bool keyword_enabled = false;
    // //
    // for(const QString& keyword: std::as_const(keywords)){
    //     if(!qobject_cast<CVEListModel*>(sourceModel())->keywords().disabled(keyword)){
    //         keyword_enabled = true;
    //         break;
    //     }
    // }
    // }

    // { !permisive if one false return false otherwise proceed
    bool keyword_enabled = true;
    for(const QString& keyword: std::as_const(keywords)){
        if(qobject_cast<CVEListModel*>(sourceModel())->keywords().disabled(keyword)){
            keyword_enabled = false;
            break;
        }
    }
    // }


    if(!keyword_enabled){
        return false;
    }

    if (filterRegularExpression().pattern().startsWith("CVE")) {
        return id.contains(filterRegularExpression());
    } else {
        foreach (const QString &keyword, keywords) {
            QRegularExpressionMatch match = filterRegularExpression().match(keyword);
            if(match.hasMatch() && !match.hasPartialMatch()){
                return true;
            }
        }
    }

    return false;
}
