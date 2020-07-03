import pandas as pd
import numpy as np
from sklearn.model_selection import KFold
from sklearn.metrics import mean_squared_error
import lightgbm as lgb
import seaborn as sns
import matplotlib.pyplot as plt
from sklearn.model_selection import train_test_split
from sklearn.metrics import plot_confusion_matrix
from sklearn import svm
from sklearn.metrics import confusion_matrix
from sklearn.feature_selection import RFE
from sklearn.svm import SVR
from sklearn.model_selection import StratifiedKFold
import graphviz
import os
os.environ["PATH"] += os.pathsep + 'C:/Program Files (x86)/Graphviz2.38/bin'

#Read CSV
X_train = pd.read_csv('datasetLeft3time.csv')
X_test = pd.read_csv('datasetRight3time.csv')
class_names = [0,1,2,3,4]




#Corr Graphs
corrmat = X_train.corr()
f, ax = plt.subplots(figsize=(20, 9))
sns.heatmap(corrmat, vmax=.8, annot=True);
plt.show()

corrmat = X_test.corr()
f, ax = plt.subplots(figsize=(20, 9))
sns.heatmap(corrmat, vmax=.8, annot=True);
plt.show()


#Drop features
y_train = X_train['group']
X_train.drop('group',axis=1, inplace=True)


y_test = X_test['group']
X_test.drop('group',axis=1, inplace=True)


X_train.drop('greenMean',axis=1, inplace=True)
X_test.drop('greenMean',axis=1, inplace=True)


X_train.drop('entro',axis=1, inplace=True)
X_test.drop('entro',axis=1, inplace=True)

X_train.drop('redMean',axis=1, inplace=True)
X_test.drop('redMean',axis=1, inplace=True)


X_train.drop('contr',axis=1, inplace=True)
X_test.drop('contr',axis=1, inplace=True)

X_train.drop('energ',axis=1, inplace=True)
X_test.drop('energ',axis=1, inplace=True)


X_train.drop('dissi',axis=1, inplace=True)
X_test.drop('dissi',axis=1, inplace=True)


X_train.drop('idmnc',axis=1, inplace=True)
X_test.drop('idmnc',axis=1, inplace=True)


categorical_features = ["month","day", "hour", "min", "sec"]
models = [];
print("Light Gradient Boosting Classifier: ")

lgbm_params =  {
    'task': 'train',
    'boosting_type': 'gbdt',
    'objective': 'multiclass',
    'num_class': 3,
    "learning_rate": 0.2,
     "num_leaves": 5,
     "max_depth": 6,
     "feature_fraction": 0.7,
     "reg_alpha": 0.15,
     "reg_lambda": 0

                }


x=0;
df_fimp = pd.DataFrame()
kf = KFold(n_splits=6,random_state=7, shuffle=True);
#kf.get_n_splits(X);
for train_index, test_index in kf.split(X_train):
    #Train dataset
    dataTrain = X_train.loc[train_index];
    trainLabel = y_train.loc[train_index];
    #Test dataset
    dataTest = X_train.loc[test_index];
    testLabel = y_train.loc[test_index];

    d_training = lgb.Dataset(dataTrain, label=trainLabel,categorical_feature=categorical_features)
    d_test = lgb.Dataset(dataTest, label=testLabel,categorical_feature=categorical_features)

    model = lgb.train(lgbm_params, train_set=d_training, num_boost_round=1000,valid_sets=[d_training, d_test])#,
                     # verbose_eval=25, early_stopping_rounds=50)

    #Feature importance Graph
    if(x==0):
        df_fimp["feature"] = X_train.columns.values
        df_fimp["importance"] = model.feature_importance()
    else:
        df_fimp_temp = pd.DataFrame()
        df_fimp_temp["feature"] = X_train.columns.values
        df_fimp_temp["importance"] = model.feature_importance()
        df_fimp.append(df_fimp_temp)
    x=x+1;
   # lgb.plot_tree(model)
   # plt.show()


    models.append(model);
plt.figure(figsize=(14, 7))
sns.barplot(x="importance", y="feature", data=df_fimp.sort_values(by="importance", ascending=False))
plt.title("LightGBM Feature Importance")
plt.tight_layout()
plt.show()
y_pred = [];
for model in models:
    if y_pred == []:
        y_pred = model.predict(X_test, num_iteration=model.best_iteration);#output is array of arrays . every label is the probabilty to get the current group.
    else:
        y_pred += model.predict(X_test, num_iteration=model.best_iteration);#output is array of arrays . every label is the probabilty to get the current group.

    print('----------------------------------------------------')
    print('----------------------------------------------------')
    print(y_pred)
    print('----------------------------------------------------')
    print('----------------------------------------------------')


predictions = []

for x in y_pred:
    predictions.append(np.argmax(x))#real predications
print('----------------------------------------------------')
print(predictions)
print('----------------------------------------------------')

cm = confusion_matrix(y_test, predictions)
print(cm)

#Accuracy
from sklearn.metrics import accuracy_score
accuracy=accuracy_score(predictions,y_test)
print(accuracy)

#Reults:
cmn = cm.astype('float') / cm.sum(axis=1)[:, np.newaxis]
df_cm = pd.DataFrame(cmn,[0,2,4] ,[0,2,4] )
plt.figure(figsize = (10,7))
sns.heatmap(df_cm, annot=True ,cmap = 'Greens', fmt = '.2g',annot_kws={"size": 40},cbar=False)
plt.title("Image Taken Every Minute",fontsize = 30)
plt.xlabel("Predicted Group",fontsize = 30)
plt.ylabel("Group",fontsize = 30)
plt.xticks(size = 30)
plt.yticks(size = 30)
plt.show()


#Get statistics about Errors and make graphs
X_test['Group'] = y_test
indices = [i for i in range(len(y_test)) if y_test[i] != predictions[i]]
print(indices)
wrong_predictions = X_test.iloc[indices,:]
print(wrong_predictions)
wrong_predictions['Wrong']=1;
print(wrong_predictions.head())

wrong0 = wrong_predictions[wrong_predictions['Group'] == 0]
wrong1 = wrong_predictions[wrong_predictions['Group'] == 1]
wrong2 = wrong_predictions[wrong_predictions['Group'] == 2]


wrong_predictions.drop('Wrong',axis=1, inplace=True)
wrong_predictions['Wrong']=0;


ax = plt.gca()

wrongPlt0 = wrong0.groupby([wrong0["hour"]])['Wrong'].sum().plot(y='Wrong',ax=ax,kind='bar',stacked=True,label="Group 0",color='blue')#y='Wrong',kind='bar',stacked=True
wrongPlt1 = wrong1.groupby([wrong1["hour"]])['Wrong'].sum().plot(y='Wrong',kind='bar',stacked=True,color='red',label="Group 2",ax=ax)
wrongPlt2 = wrong2.groupby([wrong2["hour"]])['Wrong'].sum().plot(y='Wrong',ax=ax,kind='bar',stacked=True,label="Group 4",color='yellow')
wrongPl = wrong_predictions.groupby([wrong_predictions["hour"]])['Wrong'].sum().plot(y='Wrong',kind='bar',stacked=True,color='black',label='_nolegend_',ax=ax)

plt.legend()
plt.subplots_adjust(bottom=0.22,right=0.72)
plt.xlabel('Hour')
plt.ylabel('Number of Misclassifications')
plt.title('Wrong Classifications Every Hour')
plt.show()

#More Graphes
'''
wrongPlt0 = wrong0.groupby([wrong0["month"],wrong0["day"]])['Wrong'].sum().plot(y='Wrong',ax=ax,kind='bar',stacked=True,label="Group 0",color='blue')#y='Wrong',kind='bar',stacked=True
wrongPlt1 = wrong1.groupby([wrong1["month"],wrong1["day"]])['Wrong'].sum().plot(y='Wrong',kind='bar',stacked=True,color='red',label="Group 2",ax=ax)
wrongPlt2 = wrong2.groupby([wrong2["month"],wrong2["day"]])['Wrong'].sum().plot(y='Wrong',ax=ax,kind='bar',stacked=True,label="Group 4",color='yellow')
wrongPl = wrong_predictions.groupby([wrong_predictions["month"],wrong_predictions["day"]])['Wrong'].sum().plot(y='Wrong',kind='bar',stacked=True,color='black',label='_nolegend_',ax=ax)

plt.legend()
plt.subplots_adjust(bottom=0.22,right=0.72)
plt.xlabel('(Month,Day)')
plt.ylabel('Number of Misclassifications')
plt.title('Wrong Classifications')
plt.show()
'''