import os
import glob
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import classification_report
from micromlgen import port

script_dir = os.path.dirname(os.path.abspath(__file__))
header_path = os.path.join(script_dir, "SpoilageClassifier.h")

# Map filename prefixes to class IDs
label_map = {
    "normal_": 0,
    "warning_": 1,
    "critical_": 2
}

all_dfs = []

# Automatically read and label every matching CSV in the folder
for file_path in glob.glob(os.path.join(script_dir, "*.csv")):
    filename = os.path.basename(file_path).lower()
    
    for prefix, label_id in label_map.items():
        if filename.startswith(prefix):
            temp_df = pd.read_csv(file_path)
            temp_df["label"] = label_id  # Backend automatically assigns 0, 1, or 2
            all_dfs.append(temp_df)
            print(f"Loaded {filename} -> assigned label {label_id} ({len(temp_df)} rows)")
            break

if not all_dfs:
    raise ValueError("No CSV files starting with normal_, warning_, or critical_ were found!")

# Combine all sessions into one training table
df = pd.concat(all_dfs, ignore_index=True)

X = df[["temp", "humidity", "ethylene"]].values
y = df["label"].values

# Split 80% train, 20% test
X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.2, random_state=42, stratify=y
)

# Train compact Random Forest
clf = RandomForestClassifier(
    n_estimators=15,
    max_depth=5,
    random_state=42
)
clf.fit(X_train, y_train)

# Verify accuracy
y_pred = clf.predict(X_test)
print("\nModel Evaluation:")
print(classification_report(y_test, y_pred, target_names=["Normal", "Warning", "Critical"]))

# Export to C++ header for ESP32-S3
c_code = port(clf, classname="SpoilageClassifier")
with open(header_path, "w") as f:
    f.write(c_code)

print("Exported SpoilageClassifier.h successfully.")
