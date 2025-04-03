import psycopg

from passlib.context import CryptContext
from flask_jwt_extended import JWTManager


jwt = JWTManager()
pwd = CryptContext(schemes=["bcrypt"])

conn = psycopg.connect(
    port=5432,
    user="arthur",
    host="localhost",
    dbname="postgres",
    password="12345678",
)
