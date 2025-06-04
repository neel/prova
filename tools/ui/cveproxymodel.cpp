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

    // bool keyword_enabled = true;
    // for(const QString& keyword: keywords){
    //     if(sourceModel()->keywords
    // }

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
