/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                                 *
 * ALPS Project: Algorithms and Libraries for Physics Simulations                  *
 *                                                                                 *
 * ALPS Libraries                                                                  *
 *                                                                                 *
 * Copyright (C) 2010 - 2011 by Lukas Gamper <gamperl@gmail.com>                   *
 *                                                                                 *
 * This software is part of the ALPS libraries, published under the ALPS           *
 * Library License; you can use, redistribute it and/or modify it under            *
 * the terms of the license, either version 1 or (at your option) any later        *
 * version.                                                                        *
 *                                                                                 *
 * You should have received a copy of the ALPS Library License along with          *
 * the ALPS Libraries; see the file LICENSE.txt. If not, the license is also       *
 * available from http://alps.comp-phys.org/.                                      *
 *                                                                                 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT       *
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE       *
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,     *
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER     *
 * DEALINGS IN THE SOFTWARE.                                                       *
 *                                                                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if !defined(ALPS_NGS_SCHEDULER_DUALTHREADSIM_NG_HPP) && !defined(ALPS_NGS_SINGLE_THREAD)
#define ALPS_NGS_SCHEDULER_DUALTHREADSIM_NG_HPP

#include <alps/ngs/api.hpp>

#include <boost/thread.hpp>

namespace alps {

    template<typename Impl> class dualthreadsim_ng : public Impl {
        public:
            dualthreadsim_ng(typename alps::parameters_type<Impl>::type const & p, std::size_t seed_offset = 0)
                : Impl(p, seed_offset)
            {}

            virtual bool finished() {
                return m_finished;
            }

        protected:

            template<typename T> class atomic {
                public:

                    atomic() {}
                    atomic(T const & v): value(v) {}
                    atomic(atomic<T> const & v): value(v.value) {}

                    atomic<T> & operator=(T const & v) {
                        boost::lock_guard<boost::mutex> lock(atomic_mutex);
                        value = v;
                        return *this;
                    }

                    operator T() const {
                        boost::lock_guard<boost::mutex> lock(atomic_mutex);
                        return value;
                    }

                private:

                    T volatile value;
                    boost::mutex mutable atomic_mutex;
            };

            virtual void finish() {
                m_finished = true;
            }

            // TODO: brauchen wir die protected mutex, nun haben wir 2 muteces, was alles etwas verwirrend macht?  wollen wir den nicht in den type reinwrappen?
            boost::shared_ptr<void> get_data_guard() const { return boost::shared_ptr<void>(new boost::lock_guard<boost::mutex>(native_data_mutex)); }

            boost::shared_ptr<void> get_result_guard() const { return boost::shared_ptr<void>(new boost::lock_guard<boost::mutex>(native_result_mutex)); }
        
        private:

            atomic<bool> m_finished;

            boost::mutex mutable native_data_mutex;
            boost::mutex mutable native_result_mutex;
    };

}

#endif