import web
import random

urls = (
    '/(.*)', 'hello'
)
app = web.application(urls, globals())

class hello:        
    def GET(self, name):
        if not name: 
            name = 'World'
        data = 'Hello, ' + name + '!'
        return data * random.randint(1, 1000)

if __name__ == "__main__":
    app.run()

