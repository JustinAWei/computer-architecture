import sys
import numpy as np
from scipy.stats import randint
from sklearn.model_selection import train_test_split
from sklearn import datasets
from sklearn import svm
from sklearn import tree
from sklearn.model_selection import RandomizedSearchCV

history_len = 20
filename = sys.argv[1]
trace_file = open(filename, 'r')
data = np.loadtxt(trace_file)

y = data[:,1]
X = data[:,0]

pc_X = dict()
pc_y = dict()
pc_models = dict()
for i in range(len(y)):
    if X[i] not in pc_y.keys():
        pc_y[X[i]] = []
    pc_y[X[i]].append(y[i])

for pc in pc_y.keys():
    global_hist = np.zeros(history_len)
    hist = [global_hist]
    for i in range(1,len(pc_y[pc])):
        global_hist = np.delete(global_hist, 0)
        global_hist = np.append(global_hist, pc_y[pc][i-1])
        hist.append(global_hist)
    pc_X[pc] = hist
    print(pc, len(hist), hist[0].shape)


total_accuracy = 0
total_branches = 0
for pc in pc_y.keys():
    print(pc)
    if len(pc_X[pc]) >= 0:
        branches = len(pc_y[pc])
        if len(np.unique(pc_y[pc])) <= 1:
            score = 1
        else:
            X_train, X_test, y_train, y_test = train_test_split(pc_X[pc], pc_y[pc], test_size=0.4, random_state=0)

            pc_models[pc] = svm.SVC()

            pc_models[pc].fit(X_train, y_train)
            score = pc_models[pc].score(X_test, y_test)
        total_accuracy = total_accuracy + score * branches
        total_branches = total_branches + branches
        print(score, branches, total_accuracy, total_branches)
print(total_accuracy / total_branches)
