import pickle
import sys
import pandas as pd

def SetType(l, dtype=float):
    return list(map(dtype, l))

def Predict(y_lp,rc, model="logistic"):
    df=pd.DataFrame({"Reduced_Cost":rc,"y_lp":y_lp })
    try:
        loaded_model = pickle.load(open("contrib/ML/{}.sav".format(model), 'rb'))
        df["score"] = loaded_model.decision_function(df)
        df["pred"] = loaded_model.predict(df.drop("score",axis="columns"))
    except:
        print("FATAL ERROR: Something went wrong with the prediction model. Please make sure a trained model exists or turn off the ML component.")
    df.to_csv("contrib/ML/predicted_values.csv", sep=',', encoding='utf-8', index=False)

y_lp = SetType(sys.argv[1].split(",")[:-1])
rc = SetType(sys.argv[2].split(",")[:-1])
assert(len(rc)==len(y_lp))

Predict(y_lp, rc)