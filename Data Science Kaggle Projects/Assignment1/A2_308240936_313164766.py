#Submitted by Ron Gershburg 313164766 AND Tamara Baybachov 308240936


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Imports~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from scipy.stats import skew
from scipy.special import boxcox1p
from sklearn.preprocessing import StandardScaler, RobustScaler
from sklearn.linear_model import Lasso, LassoCV


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Importing data~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
train=pd.read_csv('train.csv')
test = pd.read_csv('test.csv')
test2 = pd.read_csv('test.csv')
len_train=train.shape[0]
houses=pd.concat([train,test], sort=False)
print(train.shape)
print(test.shape)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Data visualization~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#plot missing data:
#sns.set_style("whitegrid")
#missing = train.isnull().sum()
#missing = missing[missing > 0]
#missing.sort_values(inplace=True)
#missing.plot.bar()


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Pre-Process~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#Replace missing values with 'None' string.
for col in ('Alley','Utilities','MasVnrType','BsmtQual','BsmtCond','BsmtExposure','BsmtFinType1',
            'BsmtFinType2','Electrical','FireplaceQu','GarageType','GarageFinish','GarageQual','GarageCond',
           'PoolQC','Fence','MiscFeature'):
    train[col]=train[col].fillna('None')
    test[col]=test[col].fillna('None')

#Replace missing values with the value that occured most.
for col in ('MSZoning','Exterior1st','Exterior2nd','KitchenQual','SaleType','Functional'):
    train[col]=train[col].fillna(train[col].mode()[0])
    test[col]=test[col].fillna(train[col].mode()[0])

#Replace missing values with 0.
for col in ('MasVnrArea','BsmtFinSF1','BsmtFinSF2','BsmtUnfSF','TotalBsmtSF','BsmtFullBath','BsmtHalfBath','GarageYrBlt','GarageCars','GarageArea'):
    train[col]=train[col].fillna(0)
    test[col]=test[col].fillna(0)

#Replace missing values with mean.
train['LotFrontage']=train['LotFrontage'].fillna(train['LotFrontage'].mean())
test['LotFrontage']=test['LotFrontage'].fillna(train['LotFrontage'].mean())


# drop uncorr features/ features that had many NA.
test = test.drop(['Utilities', 'Street', 'PoolQC',], axis=1)

#create new features based on other features
test['YrBltAndRemod']=test['YearBuilt']+test['YearRemodAdd']
test['TotalSF']=test['TotalBsmtSF'] + test['1stFlrSF'] + test['2ndFlrSF']

test['Total_sqr_footage'] = (test['BsmtFinSF1'] + test['BsmtFinSF2'] +
                                 test['1stFlrSF'] + test['2ndFlrSF'])

test['Total_Bathrooms'] = (test['FullBath'] + (0.5 * test['HalfBath']) +
                               test['BsmtFullBath'] + (0.5 * test['BsmtHalfBath']))

test['Total_porch_sf'] = (test['OpenPorchSF'] + test['3SsnPorch'] +
                              test['EnclosedPorch'] + test['ScreenPorch'] +
                              test['WoodDeckSF'])

# drop uncorr features/ features that had many NA.
train = train.drop(['Utilities', 'Street', 'PoolQC',], axis=1)
#create new features based on other features
train['YrBltAndRemod']=train['YearBuilt']+train['YearRemodAdd']
train['TotalSF']=train['TotalBsmtSF'] + train['1stFlrSF'] + train['2ndFlrSF']
train['Total_sqr_footage'] = (train['BsmtFinSF1'] + train['BsmtFinSF2'] +
                                 train['1stFlrSF'] + train['2ndFlrSF'])
train['Total_Bathrooms'] = (train['FullBath'] + (0.5 * train['HalfBath']) +
                               train['BsmtFullBath'] + (0.5 * train['BsmtHalfBath']))
train['Total_porch_sf'] = (train['OpenPorchSF'] + train['3SsnPorch'] +
                              train['EnclosedPorch'] + train['ScreenPorch'] +
                              train['WoodDeckSF'])

#Create Bool features.
test['haspool'] = test['PoolArea'].apply(lambda x: 1 if x > 0 else 0)
test['has2ndfloor'] = test['2ndFlrSF'].apply(lambda x: 1 if x > 0 else 0)
test['hasgarage'] = test['GarageArea'].apply(lambda x: 1 if x > 0 else 0)
test['hasbsmt'] = test['TotalBsmtSF'].apply(lambda x: 1 if x > 0 else 0)
test['hasfireplace'] = test['Fireplaces'].apply(lambda x: 1 if x > 0 else 0)
#Create Bool features for train.
train['haspool'] = train['PoolArea'].apply(lambda x: 1 if x > 0 else 0)
train['has2ndfloor'] = train['2ndFlrSF'].apply(lambda x: 1 if x > 0 else 0)
train['hasgarage'] = train['GarageArea'].apply(lambda x: 1 if x > 0 else 0)
train['hasbsmt'] = train['TotalBsmtSF'].apply(lambda x: 1 if x > 0 else 0)
train['hasfireplace'] = train['Fireplaces'].apply(lambda x: 1 if x > 0 else 0)

# drop more uncorr features/ features that had many NA.
train.drop(['GarageArea','1stFlrSF','TotRmsAbvGrd','2ndFlrSF','Alley','FireplaceQu','Fence','MiscFeature'], axis=1, inplace=True)
test.drop(['GarageArea','1stFlrSF','TotRmsAbvGrd','2ndFlrSF','Alley','FireplaceQu','Fence','MiscFeature'], axis=1, inplace=True)

#According to author.
train = train[train['GrLivArea']<4500]

len_train=train.shape[0]
#Concat train and test to same dataset
houses=pd.concat([train,test], sort=False)

#Change to catagorical.
houses['MSSubClass']=houses['MSSubClass'].astype(str)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Approximate data to Normal Distribution~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
skew=houses.select_dtypes(include=['int','float']).apply(lambda x: skew(x.dropna())).sort_values(ascending=False)
skew_df=pd.DataFrame({'Skew':skew})
skewed_df=skew_df[(skew_df['Skew']>0.5)|(skew_df['Skew']<-0.5)]

train=houses[:len_train]
test=houses[len_train:]

#Boxcox like log approximates data to normal distribution, more robust to outliers than log.
lamda=0.1 #We tried many lamdas in order to find best
for col in ('MiscVal', 'PoolArea', 'LotArea', 'LowQualFinSF', '3SsnPorch',
       'KitchenAbvGr', 'BsmtFinSF2', 'EnclosedPorch', 'ScreenPorch',
       'BsmtHalfBath', 'MasVnrArea', 'OpenPorchSF', 'WoodDeckSF',
       'LotFrontage', 'GrLivArea', 'BsmtFinSF1', 'BsmtUnfSF', 'Fireplaces',
       'HalfBath', 'TotalBsmtSF', 'BsmtFullBath', 'OverallCond', 'YearBuilt',
       'GarageYrBlt'):
    train[col]=boxcox1p(train[col],lamda)
    test[col]=boxcox1p(test[col],lamda)

#Approximate sale price normal distribution with log transform.
train['SalePrice']=np.log(train['SalePrice'])#log1p

#Make catagorical veriables to dummies/indicators in order to make catagorical values relevant.
houses=pd.concat([train,test], sort=False)
houses=pd.get_dummies(houses)

train=houses[:len_train]
test=houses[len_train:]

#Drop unused ID .
train.drop('Id', axis=1, inplace=True)
test.drop('Id', axis=1, inplace=True)

x=train.drop('SalePrice', axis=1)#Drop Target feature from train.
y=train['SalePrice']
test=test.drop('SalePrice', axis=1)

#known outliers(some from author notes and some from notebook guides)
outliers = [30, 88, 462, 631, 1322]
x = x.drop(x.index[outliers])
y = y.drop(y.index[outliers])


x = x.drop('MSSubClass_150', axis=1)
test = test.drop('MSSubClass_150', axis=1)

#Robustscalar normalizes the data so it is more robust to outliers.
sc=RobustScaler()
x=sc.fit_transform(x)
test=sc.transform(test)

#Train
model=Lasso(alpha =0.0005, random_state=1) #other alphas were tried too .
model.fit(x,y)

#Predict
pred=model.predict(test)
predFinal=np.exp(pred)#Revert the log.

#Data export
output=pd.DataFrame({'Id':test2.Id, 'SalePrice':predFinal})
output.to_csv('submission.csv', index=False)
output.head()