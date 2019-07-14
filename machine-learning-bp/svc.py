import sys
import numpy as np
from scipy.stats import randint
from sklearn.model_selection import train_test_split
from sklearn import datasets
from sklearn import svm
from sklearn.model_selection import RandomizedSearchCV

filename = sys.argv[1]
trace_file = open(filename, 'r')
data = np.loadtxt(trace_file)

X = data[:,[0]]
y = data[:,1]

clf = svm.SVC(kernel='linear', C=1)

param_dist = {"C": randint(1, 100),
              "kernel": ['linear', 'poly', 'rbf', 'sigmoid']}

# run randomized search
n_iter_search = 20
print('training')
random_search = RandomizedSearchCV(clf, param_distributions=param_dist,
                                   n_iter=n_iter_search, cv=3)

random_search.fit(X, y)
print(random_search.cv_results_)
