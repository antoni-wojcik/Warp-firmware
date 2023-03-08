import numpy as np
import matplotlib.pyplot as plt
from scipy.io import arff
import pandas as pd
import pickle
import sys

from sklearn.preprocessing import LabelEncoder
from sklearn.linear_model import Perceptron
from sklearn.linear_model import SGDClassifier
from sklearn.neural_network import MLPClassifier
from sklearn.tree import DecisionTreeClassifier
from sklearn.naive_bayes import GaussianNB
from sklearn.neighbors import KNeighborsClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report
from sklearn import tree

from sklearn.model_selection import LearningCurveDisplay
from sklearn.model_selection import KFold


NUM_FEATURES = 4


def load_data_source(dataset_path):
    X, y = None, None

    for i in range(0,51):
        dataset_path_i = dataset_path.format(1600 + i)

        data = None

        try:
            data = arff.loadarff(dataset_path_i)
        except:
            print("WARNING: PATH DOES NOT EXIST: " + dataset_path_i)
            continue
        
        df = pd.DataFrame(data[0])

        y_i = df['"ACTIVITY"'].to_numpy()

        columns = []
        if(NUM_FEATURES > 1):
            #columns.extend(list(range(31,46,1)))
            #columns.extend(list(range(31,34,1)))
            columns.extend(list(range(40,43,1)))
            
        columns.extend([91])

        df = df.iloc[:, columns]

        if i == 0:
            print(df.columns)

        X_i = df.to_numpy()

        if i == 0:
            y = y_i
            X = X_i
        else:
            y = np.hstack((y, y_i))
            X = np.vstack((X, X_i))

    # Change the char representation of labels to float
    y = LabelEncoder().fit_transform(y)

    idx = (y <= 4).nonzero()

    y = y[idx]
    if(NUM_FEATURES > 1):
        X = np.squeeze(X[idx, :])
    else:
        X = np.squeeze(X[idx])

    remove_stairs = True

    if(remove_stairs):
        idx = (y != 2).nonzero()

        y = y[idx]
        if(NUM_FEATURES > 1):
            X = np.squeeze(X[idx, :])
        else:
            X = np.squeeze(X[idx])

    merge_cases = True

    if(merge_cases):
        #y[y == 1] = 0
        y[y == 4] = 3

    y = LabelEncoder().fit_transform(y)

    if(NUM_FEATURES == 1):
        X = np.array([X]).T

    return X, y

def plot_convergence(clf, X, y):
    font = {'size' : 12}
    plt.rc('font', **font)
    
    fig, ax = plt.subplots(1, 1, figsize=(5, 4))
    fig.subplots_adjust(bottom=0.2, left=0.2)

    common_params = {
        "X": X,
        "y": y,
        "train_sizes": np.linspace(0.1, 1.0, 5),
        "cv": KFold(n_splits=20),
        "score_type": "both",
        "n_jobs": -1,
        "line_kw": {"marker": "o"},
        "std_display_style": "fill_between",
        "score_name": "Accuracy",
    }

    LearningCurveDisplay.from_estimator(clf, **common_params, ax=ax)
    handles, label = ax.get_legend_handles_labels()
    ax.legend(handles[:2], ["Training Score", "Test Score"])
    ax.set_title(f"Learning Curve for {clf.__class__.__name__}")

    plt.show()

def combine_acc_gyro(y_acc, y_gyro, X_acc, X_gyro):
    X, y = None, None

    i, j = 0, 0

    while(i < len(y_acc) and j < len(y_gyro)):
        if(y_acc[i] == y_gyro[j]):
            y_line = y_acc[i]

            X_line = np.hstack((X_acc[i, :], X_gyro[j, :]))

            if(i == 0 and j == 0):
                y = y_line
                X = X_line
            else:
                y = np.hstack((y, y_line))
                X = np.vstack((X, X_line))

            i += 1
            j += 1
        else:
            if(y_acc[i] != y_acc[i-1]):
                j += 1
            else:
                i += 1

    return X, y


def main():
    X_1, y_1 = load_data_source('/Users/antoni/Documents/CAMBRIDGE/MRes/Courses/Embedded_Systems_for_the_Internet_of_Things/Coursework/Coursework_4/wisdm-dataset/arff_files/watch/accel/data_{}_accel_watch.arff')

    #X_2, y_2 = load_data_source('/Users/antoni/Documents/CAMBRIDGE/MRes/Courses/Embedded_Systems_for_the_Internet_of_Things/Coursework/Coursework_4/wisdm-dataset/arff_files/phone/gyro/data_{}_gyro_phone.arff')

    #X, y = combine_acc_gyro(y_1, y_2, X_1, X_2)

    #X, y = X_1, y_1

    X, y = X_1, y_1

    print(X)
    print(y)

    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.1, random_state=42)

    #clf = SVC(gamma='auto')

    #clf = MLPClassifier(hidden_layer_sizes = 100)
    clf = GaussianNB()
    #clf = KNeighborsClassifier(n_neighbors=1)

    #clf = Perceptron(tol=1e-4, random_state=42, class_weight="balanced")
    #clf = DecisionTreeClassifier(random_state=42, max_depth=6, max_leaf_nodes=12)

    clf.fit(X_train, y_train)

    # Generate the convergence plot for the model
    #plot_convergence(clf, X_train, y_train)

    y_pred = clf.predict(X_test)

    # Print the classification report 
    print(classification_report(y_test, y_pred))  

    #p = pickle.dumps(clf)
    #print(sys.getsizeof(p))

    idx = 13

    X_t_1 = X_test[idx, :]
    y_t_1 = y_test[idx]

    priors = clf.class_prior_
    means = clf.theta_
    sds = np.sqrt(clf.var_)

    print(means)
    print(sds)

    show_prob = False

    if(show_prob):
        prob = np.zeros((4))

        for i in range(4):
            pbs = normal_dist(X_t_1, means[i, :], sds[i, :])

            #print(pbs)

            #j = 0
            #print(normal_dist(X_t_1[j], means[i, j], sds[i, j]))

            prob[i] = np.prod(pbs) * priors[i]

        prob = prob / np.sum(prob)

        print(prob)

        print(y_t_1)

        print(clf.predict_proba(X_test[idx:idx+1, :]))

    

    #for i in range(clf.n_features_in_):



    #text_representation = tree.export_text(clf)
    #print(text_representation)

    #plt.figure()
    #plot_tree(clf)
    #plt.show()

    #coefs = np.array(clf.coef_)

    #np.savetxt('/Users/antoni/Documents/CAMBRIDGE/MRes/Courses/Embedded_Systems_for_the_Internet_of_Things/Coursework/Coursework_4/coefs.csv', coefs, delimiter=",")

    #y_probs = np.dot(X_test, np.transpose(coefs))
    #y_probs -= np.max(y_probs, axis = 1)[:,None]
    #print(y_probs[0:20, :])
    #print(np.sum(np.exp(y_probs), axis = 1)[0:20])
    #y_probs = np.exp(y_probs)/np.sum(np.exp(y_probs), axis = 1)[:,None] 

    #print(y_probs[0:20, :])
    #y_pred2 = np.argmax(y_probs, axis = 1)
    #print(y_pred2[0:20])
    #print(y_test[0:20])

    #print(classification_report(y_test, y_pred2)) 

    #print(clf.get_params())

def test():
    mean = [13.42378107, 9.76847904]
    sd =   [3.30213513, 0.21466643]

    print(normal_dist(9.959, mean[0], sd[0]))
    print(normal_dist(9.959, mean[1], sd[1]))

def normal_dist(x, mean, sd):
    prob_density = (1/(sd*np.sqrt(2*np.pi))) * np.exp(-0.5*((x-mean)/sd)**2)
    
    return prob_density

if __name__ == "__main__":
    main()