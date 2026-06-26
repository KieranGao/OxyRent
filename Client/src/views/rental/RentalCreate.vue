<template>
  <div class="page-container">
    <div class="page-header">
      <h2>创建租赁订单</h2>
      <p>预订租赁车辆</p>
    </div>

    <el-card>
      <el-form
        ref="formRef"
        :model="form"
        :rules="rules"
        label-position="top"
        style="max-width: 560px"
      >
        <el-form-item label="车辆" prop="vehicle_id">
          <el-select
            v-model="form.vehicle_id"
            placeholder="请选择车辆"
            filterable
            style="width: 100%"
            :loading="vehiclesLoading"
          >
            <el-option
              v-for="v in vehicleOptions"
              :key="v.id"
              :label="`${v.plate_number} - ${v.brand} ${v.model}`"
              :value="v.id"
            />
          </el-select>
        </el-form-item>

        <div class="form-row">
          <el-form-item label="开始日期" prop="start_date" style="flex: 1">
            <el-date-picker
              v-model="form.start_date"
              type="date"
              placeholder="请选择开始日期"
              value-format="YYYY-MM-DD"
              style="width: 100%"
              :disabled-date="(date) => date < new Date()"
            />
          </el-form-item>
          <el-form-item label="结束日期" prop="end_date" style="flex: 1">
            <el-date-picker
              v-model="form.end_date"
              type="date"
              placeholder="请选择结束日期"
              value-format="YYYY-MM-DD"
              style="width: 100%"
              :disabled-date="(date) => date < new Date(form.start_date || undefined)"
            />
          </el-form-item>
        </div>

        <el-form-item label="支付方式" prop="payment_method">
          <el-radio-group v-model="form.payment_method">
            <el-radio value="balance">余额支付</el-radio>
            <el-radio value="qr">扫码支付</el-radio>
          </el-radio-group>
        </el-form-item>

        <el-form-item v-if="form.payment_method === 'balance'" label="当前余额">
          <span :style="{ color: authStore.balance > 0 ? 'var(--accent)' : 'var(--text-tertiary)' }">
            {{ formatMoney(authStore.balance) }}
          </span>
        </el-form-item>

        <el-form-item label="备注">
          <el-input v-model="form.notes" type="textarea" :rows="3" placeholder="选填" />
        </el-form-item>

        <el-form-item>
          <el-button type="primary" :loading="submitting" @click="handleSubmit">
            创建订单
          </el-button>
          <el-button @click="$router.back()">取消</el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import { getVehicleList } from '@/api/vehicle'
import { createRental } from '@/api/rental'
import { getBalance } from '@/api/user'
import { ElMessage } from 'element-plus'

const route = useRoute()
const router = useRouter()
const authStore = useAuthStore()
const formRef = ref(null)
const submitting = ref(false)
const vehiclesLoading = ref(false)
const vehicleOptions = ref([])

function formatMoney(val) {
  return (val || 0).toLocaleString('zh-CN', { style: 'currency', currency: 'CNY' })
}

async function fetchBalance() {
  try {
    const res = await getBalance()
    if (res.error === 0) {
      authStore.setBalance(res.balance || 0)
    }
  } catch {
    // ignore
  }
}

const form = reactive({
  vehicle_id: route.query.vehicle_id ? Number(route.query.vehicle_id) : null,
  start_date: '',
  end_date: '',
  payment_method: 'qr',
  notes: '',
})

const rules = {
  vehicle_id: [{ required: true, message: '请选择车辆', trigger: 'change' }],
  start_date: [{ required: true, message: '请选择开始日期', trigger: 'change' }],
  end_date: [{ required: true, message: '请选择结束日期', trigger: 'change' }],
  payment_method: [{ required: true, message: '请选择支付方式', trigger: 'change' }],
}

async function loadVehicles() {
  vehiclesLoading.value = true
  try {
    const res = await getVehicleList({ page: 1, page_size: 100, status: 'available' })
    if (res.error === 0) {
      vehicleOptions.value = res.list || res.vehicles || []
    }
  } catch {
    // ignore
  } finally {
    vehiclesLoading.value = false
  }
}

async function handleSubmit() {
  const valid = await formRef.value.validate().catch(() => false)
  if (!valid) return

  if (form.payment_method === 'balance') {
    if (authStore.balance <= 0) {
      ElMessage.error('余额不足，请先充值或选择扫码支付')
      return
    }
  }

  submitting.value = true
  try {
    const res = await createRental({
      user_id: authStore.uid,
      vehicle_id: form.vehicle_id,
      start_date: form.start_date,
      end_date: form.end_date,
      payment_method: form.payment_method,
      notes: form.notes,
    })
    if (res.error === 0) {
      ElMessage.success('租赁订单已创建')
      router.push('/rentals')
    } else {
      ElMessage.error(res.msg || '创建订单失败')
    }
  } catch {
    ElMessage.error('创建订单失败')
  } finally {
    submitting.value = false
  }
}

onMounted(() => {
  loadVehicles()
  fetchBalance()
})
</script>

<style scoped>
.form-row {
  display: flex;
  gap: 20px;
}
</style>
