Import("env")
import os

def after_upload(source, target, env):
    print("Uploading filesystem image...")
    os.system("pio run --target uploadfs")

env.AddPostAction("upload", after_upload)
