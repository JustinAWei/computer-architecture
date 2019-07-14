import sys
import numpy as np
from scipy.stats import randint
from sklearn.model_selection import train_test_split
from sklearn import datasets
from sklearn import svm
from sklearn.model_selection import RandomizedSearchCV
from sklearn.neighbors import KNeighborsClassifier

filename = sys.argv[1]
trace_file = open(filename, 'r')
data = np.loadtxt(trace_file)

X = data[:,[0]]
y = data[:,1]

clf = KNeighborsClassifier()

# specify parameters and distributions to sample from
param_dist = {"n_neighbors": randint(1, 30000),
              "weights": ['uniform', 'distance']}

# run randomized search
n_iter_search = 20
print('training')
random_search = RandomizedSearchCV(clf, param_distributions=param_dist,
                                   n_iter=n_iter_search, cv=3)

random_search.fit(X, y)
print(random_search.cv_results_)
