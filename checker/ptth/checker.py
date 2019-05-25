import os
import nclib
import re
import string
import random
import subprocess

from ctf_gameserver.checker import BaseChecker
from ctf_gameserver.checker.constants import OK, NOTWORKING, NOTFOUND

SCRIPTDIR = os.path.dirname(os.path.realpath(__file__))
BORKEDMD5 = os.path.join(SCRIPTDIR, "borkedmd5")

letters = list(string.ascii_letters)
printable = list(string.printable)
nonprintable = [chr(c) for c in range(0x20)]
printable.extend(nonprintable) 
printable.remove('\n')
printable.remove('\n')

resources = [
        """Adamantium is a fictional metal alloy appearing in American comic books published by Marvel Comics. It is best known as the substance bonded to the character Wolverine's skeleton and claws.""",
        """Kryptonite is a fictional material that appears primarily in Superman stories. In its most well-known form, it is a green, crystalline material that emits a peculiar radiation that weakens Superman, but is generally harmless to humans when exposed to it in short term, however, when it gets into their bloodstream it can poison them. There are other varieties of kryptonite such as red and gold kryptonite which have different but still generally negative effects on Superman.""",
        """Vibranium is a fictional metal appearing in American comic books published by Marvel Comics, noted for its extraordinary abilities to absorb, store, and release large amounts of kinetic energy. Mined only in Wakanda, the metal is associated with Black Panther, who wears a suit of vibranium, and with Captain America, who bears a vibranium shield.""",
        """Dark matter is a hypothetical form of matter that is thought to account for approximately 85% of the matter in the universe and about a quarter of its total energy density. The majority of dark matter is thought to be non-baryonic in nature, possibly being composed of some as-yet undiscovered subatomic particles."""
        """Beer is one of the oldest and most widely consumed alcoholic drinks in the world, and the third most popular drink overall after water and tea. Beer is brewed from cereal grains—most commonly from malted barley, though wheat, maize (corn), and rice are also used. During the brewing process, fermentation of the starch sugars in the wort produces ethanol and carbonation in the resulting beer.""",
        """This metal, which is found solely on Paradise Island, is the indestructible metal out of which Wonder Woman's bracelets are made. Wonder Woman, and other inhabitants of Paradise Island, use the bracelets to deflect bullets. The material was featured in the Wonder Woman television series in its fifth episode, "The Feminum Mystique Part 2", which aired on 8 November 1976.""",
        """A fictional metal that appears in the Marvel Universe, utilized by the residents of Asgard. Uru has the unique properties of being both highly durable and capable of holding enchantments. It has been noted that only a god can safely apply permanent enchantments. Uru's durability makes it nigh-impossible to work with, requiring either the heart of a star or an enchanted forge. Notable objects made of Uru include Thor's hammer Mjolnir and Beta Ray Bill's weapon, Stormbreaker.""",
        """In the Marvel Comics universe, Carbonadium is a form of Adamantium that is developed the USSR and used by the villain Omega Red, whose retractable metal tentacles are composed of the radioactive metal alloy. Carbonadium is nearly as strong as Adamantium, but more flexible. It is also used in the armor suit of Moon Knight in the third series of that comic.""",
        """A fictional metal alloy that appears in the DC Universe, a heavy isotope of iron, Fe676 It is native to Thanagar, the home planet of Katar Hol and Shayera Thal, the Silver Age Hawkman and Hawkwoman. Among the unusual properties of Nth metal is the ability to negate gravity, allowing a person wearing an object made of Nth metal, such as a belt, to fly. In addition, Nth metal also protects the wearer from the elements and speeds the healing of wounds, increases their strength, and protects them from extremes in temperature. It has many other properties that have yet to be revealed in full.""",
        """An artificial element from the DC Universe, created by Dayton Industries. Promethium was named after the eponymous titan of Greek myth Prometheus, for his deed of giving mankind fire and knowledge erstwhile being chained to a rock with eagles picking at his innards for all eternity after. Promethium is a metal alloy that comes in two quite different isotopes, "depleted" and "volatile" promethium. Promethium's properties make it highly coveted by various public and clandestine interests. Key among which is that it's virtually indestructible with self restorative properties. The mercenary Deathstroke, for example, uses a suit of the volatile variety which could mend itself after being damaged."""
        ]

class Ptth(BaseChecker):
    def __init__(self, tick, team, service, ip):
        super().__init__(tick, team, service, ip)
        self.ip = ip
        self.port = 31337

    def is_correct_hcookie(self, cookie, response):
        try:
            r = response.split(b'\r\n')
            hcookie = r[2][9:]
            self.logger.debug("hcookie: {}".format(hcookie))
            self.logger.debug("cookie: {}".format(cookie))
            p = subprocess.Popen([BORKEDMD5, cookie.decode()], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
            h = p.stdout.read()
            self.logger.debug("calced hcookie: {}".format(h))
        except Exception as e:
            self.logger.debug(e)
            return False

        if h != hcookie:
            return False
        return True

    def place_flag(self):
        cookie = self.get_cookie()
        if not cookie:
            return NOTWORKING

        self._put('cookie', self.tick, cookie)

        content = bytes(self.get_flag(self.tick), 'ascii')
        random.shuffle(letters)
        path = bytes("".join(letters[:random.randint(8, 16)]), 'ascii')
        self._put('path', self.tick, path)

        r = self.post_file(cookie, path, content)

        if not self.is_correct_hcookie(cookie, r):
            self.logger.debug("hcookie wrong")
            return NOTWORKING

        return OK

    def check_flag(self, tick):
        cookie = self._get('cookie', tick)
        path = self._get('path', tick)

        flag = bytes(self.get_flag(tick), 'ascii')

        self.logger.debug(path)
        r = self.get_file(cookie, path)

        if not self.is_correct_hcookie(cookie, r):
            self.logger.debug("hcookie wrong")
            return NOTWORKING

        tmp = r.split(b'\r\n')
        if tmp and len(tmp) > 0:
            if flag in tmp[-1]:
                return OK
        else:
            return NOTWORKING
        return NOTFOUND

    def check_service(self):

        # create files
        cookie = self.get_cookie()
        if not cookie:
            return NOTWORKING

        random.shuffle(printable)
        content = bytes(self.randomString(), 'utf-8')
        size = len(content)
        if len(content) > 512:
            c1 = content[:512-34]
            c2 = content[512-34:]
            c2 = c2.decode().replace('%', '3').encode()
            content = c1 + c2

        random.shuffle(letters)
        path = bytes("".join(letters[:random.randint(8, 16)]), 'utf-8')

        r1 = self.post_file(cookie, path, content)

        if not self.is_correct_hcookie(cookie, r1):
            self.logger.debug("hcookie wrong")
            return NOTWORKING

        r2 = self.get_file(cookie, path)

        if not self.is_correct_hcookie(cookie, r2):
            self.logger.debug("hcookie wrong")
            return NOTWORKING

        # truncate up to nullbyte
        content = content[:content.find(b'\0')]
        size = len(content)
        if size < 512:
            if r2.find(content) == -1:
                self.logger.debug("content not here")
                self.logger.debug(r2)
                return NOTWORKING
        else:
            if bytes('truncating input at:', 'utf-8') not in r2:
                self.logger.debug("truncating input broken")
                return NOTWORKING
            truncated = r2.split(bytes('input at:', 'utf-8'))[1]
            if content.find(truncated) == -1:
                self.logger.debug("couldn't find truncated {} in {}".format(truncated, content))
                return NOTWORKING

        # check directory listing
        r3 = self.get_listdir(cookie)
        if r3.find(cookie) == -1 or r3.find(path) == -1:
            self.logger.debug("dirlisting broken")
            return NOTWORKING

        return OK

    def get_listdir(self, cookie):
        nc = nclib.Netcat((self.ip, self.port), verbose=True)
        nc.send('TEG / PTTH\\1.1\n'.encode())
        nc.send('eikooc: {}\n\n'.format(cookie.decode()).encode())
        nc.shutdown_wr()
        r = nc.recv()
        return r

    def post_file(self, cookie, fname, content):
        nc = nclib.Netcat((self.ip, self.port), verbose=True)
        nc.send('TSOP /{} PTTH\\1.1\n'.format(fname.decode()).encode())
        nc.send('eikooc: {}\n\n'.format(cookie.decode()).encode())
        nc.send('{}\n'.format(content.decode()).encode())
        nc.shutdown_wr()
        r = nc.recv()
        return r

    def get_file(self, cookie, fname):
        nc = nclib.Netcat((self.ip, self.port), verbose=True)
        nc.send('TEG /{} PTTH\\1.1\n'.format(fname.decode()).encode())
        nc.send('eikooc: {}\n\n'.format(cookie.decode()).encode())
        nc.shutdown_wr()
        r = nc.recv()
        return r

    def get_cookie(self):
        nc = nclib.Netcat((self.ip, self.port), verbose=True)
        #nc.echo_hex = True
        nc.send(b'TEG / PTTH\\1.1\n\n')
        nc.shutdown_wr()
        r = nc.recv()
        #self.logger.debug(r)
        m = re.match(".*?Index of (.*?)<", str(r))
        if not m or len(m.groups()) != 1:
            return None
        return bytes(m.group(1), 'utf-8')

    def _key_tick(self, key, tick):
        return '{}_{:03d}'.format(key, tick)

    def _put(self, key, tick, value):
        self.logger.debug('PUT %s@%d %s', repr(key), tick, repr(value))
        return self.store_blob(self._key_tick(key, tick), value)

    def _get(self, key, tick):
        value = self.retrieve_blob(self._key_tick(key, tick))
        if value is not None:
            return value

    def randomString(self):

        if random.random() < 0.6:
            stringLength = random.choice([32, 64, 128, 256, 564])
            return ''.join(random.choice(printable) for i in range(stringLength))
        else:
            return random.choice(resources)
