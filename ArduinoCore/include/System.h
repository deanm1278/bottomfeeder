/*******************************************************************************
 * Copyright (C) Lawrence Lo (https://github.com/galliumstudio). 
 * All rights reserved.
 *
 * This program is open source software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

#ifndef SYSTEM_H
#define SYSTEM_H

#include "qpcpp.h"
#include "fw_macro.h"
#include "fw_evt.h"
#include "fw_pipe.h"

enum {
    SYSTEM = 1,
    UART2_ACT,
    UART2_IN,
    UART2_OUT,
    USER_BTN,
    USER_LED,
    HSM_COUNT
};

// Higher value corresponds to higher priority.
// The maximum priority is defined in qf_port.h as QF_MAX_ACTIVE (32)
enum
{
    PRIO_UART2_ACT  = 30,
    PRIO_CONSOLE    = 28,
    PRIO_SYSTEM     = 26,
    PRIO_USER_BTN   = 24,
    PRIO_USER_LED   = 22,
    PRIO_SAMPLE     = 5
};

using namespace QP;
using namespace FW;

class System : public QActive {
public:
    System();
    void Start(uint8_t prio) {
        QActive::start(prio, m_evtQueueStor, ARRAY_COUNT(m_evtQueueStor), NULL, 0);
    }

protected:
    static QState InitialPseudoState(System * const me, QEvt const * const e);
    static QState Root(System * const me, QEvt const * const e);
    static QState Stopped(System * const me, QEvt const * const e);
    static QState Starting1(System * const me, QEvt const * const e);
    static QState Starting2(System * const me, QEvt const * const e);
    static QState Stopping1(System * const me, QEvt const * const e);
    static QState Stopping2(System * const me, QEvt const * const e);
    static QState Started(System * const me, QEvt const * const e);

    void HandleCfm(ErrorEvt const &e, uint8_t expectedCnt);

    enum {
        EVT_QUEUE_COUNT = 16,
        DEFER_QUEUE_COUNT = 4
    };
    QEvt const *m_evtQueueStor[EVT_QUEUE_COUNT];
    QEvt const *m_deferQueueStor[DEFER_QUEUE_COUNT];
    QEQueue m_deferQueue;
    uint8_t m_id;
    char const * m_name;
    uint16_t m_nextSequence;
    uint16_t m_savedInSeq;
    uint8_t m_cfmCount;
  
  enum {
        UART_OUT_FIFO_ORDER = 11,
        UART_IN_FIFO_ORDER = 10
    };
    uint8_t m_uart2OutFifoStor[1 << UART_OUT_FIFO_ORDER];
    uint8_t m_uart2InFifoStor[1 << UART_IN_FIFO_ORDER];
    Fifo m_uart2OutFifo;
    Fifo m_uart2InFifo;
  
  QTimeEvt m_stateTimer;
    QTimeEvt m_testTimer;
    
};


#endif // SYSTEM_H


