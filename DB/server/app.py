from ext import jwt
from flask import Flask
from flask_cors import CORS
from datetime import date, timedelta
from flask.json.provider import DefaultJSONProvider
from flask.json.provider import _default as json_default


json_custom = lambda o: o.isoformat() if isinstance(o, date) else json_default(o)
DefaultJSONProvider.default = staticmethod(json_custom)


def app_init():
    app = Flask(__name__)
    app.config["JWT_SECRET_KEY"] = "arthur"
    app.config["JWT_ACCESS_TOKEN_EXPIRES"] = timedelta(days=7)

    CORS(app)
    jwt.init_app(app)

    from teacher import bp as teacher
    from student import bp as student

    app.register_blueprint(teacher)
    app.register_blueprint(student)

    return app


if __name__ == "__main__":
    app = app_init()
    app.run()


# class JSONProvider(DefaultJSONProvider):
#     @staticmethod
#     def custom(o):
#         if isinstance(o, date):
#             return o.isoformat()
#         return DefaultJSONProvider.default(o)
#     default = custom

# app.json = JSONProvider(app)
