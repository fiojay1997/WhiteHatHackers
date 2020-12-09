import botocore
import boto3


AWS_ACCESS_KEY_ID = 'AKIAJZJIQW4K3FHBTKOQ'
AWS_SECRETE_KEY = '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' 

def download_img_from_s3(bucket_name, key):
    session = boto3.Session(
        aws_access_key_id = AWS_ACCESS_KEY_ID,
        aws_secret_access_key = AWS_SECRETE_KEY,
    )
    s3 = session.resource('s3')

    try:
        s3.Bucket(bucket_name).download_file(key, KEY + '.png')
    except botocore.exceptions.ClientError as e:
        if e.response['Error']['Code'] == "404":
            print("The object does not exist.")
        else:
            raise


def proceed():

    
if __name__ == "__main__":
    download_img_from_s3()