import sys
import numpy as np
from scipy.stats import randint
from sklearn.model_selection import train_test_split
from sklearn import datasets
from sklearn.neighbors import KNeighborsClassifier
from sklearn.model_selection import RandomizedSearchCV

history_len = 20
filename = sys.argv[1]
trace_file = open(filename, 'r')
# trace_file = open('data.txt', 'r')
data = np.loadtxt(trace_file)

y = data[:,1]
X = []

global_hist = np.zeros(history_len)

X.append(global_hist)
for i in range(1,len(y)):
    global_hist = np.delete(global_hist, 0)
    global_hist = np.append(global_hist, y[i-1])
    X.append(global_hist)

clf = KNeighborsClassifier()

# specify parameters and distributions to sample from
param_dist = {"n_neighbors": randint(1, 30000)}

# run randomized search
n_iter_search = 20
print('training')
random_search = RandomizedSearchCV(clf, param_distributions=param_dist,
                                   n_iter=n_iter_search, cv=3, n_jobs=-1)

random_search.fit(X, y)
print(random_search.cv_results_)
